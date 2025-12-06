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

### Events
`EventBus` is a typed bus keyed by `std::type_index`.

- `publish(e)` dispatches immediately; `enqueue(e)` + `flush()` defers delivery (events enqueued
  by handlers during a flush wait for the next flush).
- Handlers may unsubscribe during dispatch (takes effect immediately, storage is compacted after
  the outermost dispatch). Handlers added during dispatch only receive later events.
- Single-threaded by design; no locking.
