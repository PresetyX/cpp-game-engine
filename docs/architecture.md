# Architecture

Work in progress; sections are added as modules are implemented.

## Decisions

- **Modules as static libraries** (`engine_core`, later `engine_ecs`, `engine_physics`, ...),
  each with an `engine::<name>` alias. GPU-free logic stays in libraries that tests can link
  without creating a window.
- **Dependencies on demand.** `cmake/Dependencies.cmake` only fetches what existing modules use
  (currently GoogleTest). Each library is pinned by tag.
- **Own logger, no spdlog.** `engine/core/log.hpp` is a small level-filtered logger with an
  injectable sink (tests capture output; default is stderr). One less dependency.
- **Time in `double` seconds.** Tests use powers of two (1/64) so comparisons are exact.
- **Warnings policy.** `-Wall -Wextra -Wpedantic -Werror` (MSVC: `/W4 /WX`) applied via
  `engine_set_warnings()` to our own targets only.

## Core

### Fixed timestep and interpolation
`FixedTimestep` is a pure accumulator: it never reads a clock, the caller passes the frame time
(`FrameTimer::tick()` in an app). Each frame: `advance(frame_dt)` returns how many fixed
simulation steps are due, and `alpha()` (leftover / step, in [0, 1)) is used to interpolate
rendering between the previous and current simulation state.

- Why a fixed step: physics and gameplay behave identically regardless of frame rate, and tests
  can replay exact sequences.
- Frame time is clamped to `max_frame_time` (default 0.25 s) so a long stall (breakpoint, window
  drag) cannot cause a runaway catch-up loop ("spiral of death"); the simulation just slows down.
- Keeping the clock out of the class is what makes it deterministic and unit-testable.

### Events (core)
`EventBus` is a typed bus keyed by `std::type_index`.

- `publish(e)` dispatches immediately; `enqueue(e)` + `flush()` defers delivery (events enqueued
  by handlers during a flush wait for the next flush).
- Handlers may unsubscribe during dispatch (takes effect immediately, storage is compacted after
  the outermost dispatch). Handlers added during dispatch only receive later events.
- Single-threaded by design; no locking.

## ECS

### Entities
`Entity` is `{uint32_t index, uint32_t generation}` (8 bytes). `EntityPool` keeps one slot per
index (`generation`, `alive`) and a LIFO free list. `destroy()` bumps the slot generation, so a
stale handle to a reused index fails `alive()` and every component lookup. A slot whose
generation reaches `UINT32_MAX` is retired instead of reused (this branch is not covered by a
test: it would need 2^32 create/destroy cycles on one slot). A packed 32-bit handle (20/12 bits)
was rejected because 4096 reuses would wrap the generation and revive stale handles.

### Storage: one sparse set per component type
Each `SparseSet<T>` has `sparse` (entity index -> dense position), `dense` (entities) and
`values` (components). Insert, remove and lookup are O(1); removal is swap-and-pop, so components
stay contiguous and iteration order is unspecified. Dense entries store the full handle, so
lookups also check the generation. `Registry` keys pools by `std::type_index` (same pattern as
`EventBus`); `add`, `get` and `has` pay one hash lookup, while a `View` caches the pool pointers.

Why sparse sets instead of archetypes: much simpler (no table moves when a component is added or
removed), add/remove are O(1), and iterating a single component is a linear scan. Trade-off:
multi-component views do a membership check per entity of the smallest pool instead of scanning
perfectly packed tables, and the components of one entity are not adjacent in memory.

### Views
`registry.view<A, const B>()` iterates the smallest pool (chosen when the view is built)
backwards and yields entities that have all the components; `each(fn)` accepts
`fn(Entity, A&, const B&)` or `fn(A&, const B&)`. A const registry yields read-only views.
Component types in a view must be distinct. A component type that was never added gives an empty
view. Views hold raw pool pointers: build them right before iterating.

During iteration it is allowed to modify component values, create entities (they are not
visited), add or remove components of, or destroy, the *current* entity, and add components of
types outside the view. Changing the viewed pools for *other* entities is memory-safe but may
skip or repeat entities: defer those changes until after the loop. Component references are
invalidated by any add or remove on the same pool. Single-threaded.

### Error handling
`Registry::add` on a dead entity throws `std::invalid_argument`. `try_get` returns null, and
`destroy`/`remove`/`has` on dead entities return false. `get` (registry, sparse set and view) is
unchecked in release builds; it only has an `assert`.

### Known limitations
- The sparse array is not paged: 4 bytes per entity index per component type, sized by the
  highest index used.
- No component groups, sorting, deferred command buffer or add/remove events.
- Components must be move constructible and move assignable; wrap `bool` in a struct.
- Measured numbers live in `docs/benchmarks.md`; nothing beyond them is claimed.
