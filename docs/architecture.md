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

## Math

`engine_math` is a header-only INTERFACE library over GLM 1.0.1 (core and gtc headers only,
included through `engine/math/glm.hpp`). `Transform` holds `vec3 position`, `quat rotation`,
`vec3 scale`; `to_matrix()` composes T * R * S. `PreviousTransform` stores the transform of the
previous fixed step and `interpolate(prev, curr, alpha)` (lerp for position and scale, slerp for
rotation) is what rendering uses with `FixedTimestep::alpha()`. `Aabb` has `overlaps` (touching
counts, for the broadphase) and `contains`.

## Physics

`engine_physics` (static library, depends on `engine_ecs` and `engine_math`). Physics is 3D,
translation only: rotation and `Transform::scale` are never read or written by it.

Components (`engine/physics/components.hpp`):
- `Collider`: `Sphere` (radius) or `Box` (half extents, axis-aligned in world space), plus
  `friction` (Coulomb coefficient) and `restitution`. Dimensions come from the collider, not
  from `Transform::scale`.
- `RigidBody`: `velocity`, accumulated `force`, `inverse_mass`, `linear_damping`,
  `gravity_scale`. An entity with a `Collider` and no `RigidBody` is static. `inverse_mass == 0`
  is kinematic: no gravity or forces, still moves with its velocity, infinite mass for the solver.

Integration (`engine/physics/integrate.hpp`), called once per fixed step in this order:
1. `store_previous_transforms(registry)` copies `Transform` into `PreviousTransform` (only for
   entities that have both; add `PreviousTransform` to anything that must be interpolated).
2. `integrate_bodies(registry, dt, gravity)`: semi-implicit Euler, `v += a*dt` first, then
   `x += v*dt` with the new `v`. Damping is `v *= 1/(1 + c*dt)`, which stays stable for any `dt`.
   `force` is cleared after each step, so forces must be re-applied every step.

### Narrowphase (`engine/physics/narrowphase.hpp`)
Pure functions (no Registry) returning `std::optional<Contact>`: `collide_sphere_sphere`,
`collide_sphere_box`, `collide_box_box` and `collide(Collider, pos, Collider, pos)`, which
dispatches on `ShapeType` (Box-Sphere is Sphere-Box with the normal flipped).

- `Contact { normal, depth, point }`: `normal` is unit length and points from A to B (the solver
  pushes B along `+normal`, A along `-normal`); `depth > 0` always; `point` is the midpoint
  between the deepest point of A inside B and the surface of B along `-normal`.
- Exact touching (zero penetration) is not a contact: no epsilon, tests use `>=`/`<=`. The
  broadphase accepts touching AABBs; the narrowphase discards them.
- Sphere-sphere: coincident centers use normal `+Y`, `depth = ra + rb`.
- Sphere-box: center outside uses the closest point on the box. Center inside or exactly on the
  surface leaves through the nearest face (order x-, x+, y-, y+, z-, z+; first wins ties), with
  `depth = r + distance_to_face`; continuous with the outside case (center on a face gives
  `depth = r`).
- Box-box: axis of least overlap (ties x < y < z); sign of the center delta gives the normal
  (`+` when the delta is 0); `point` is the center of the intersection box.
- A/B symmetry: swapping gives the same `depth` and `point` with the normal negated. Degenerate
  cases (coincident centers, zero delta on the chosen axis) give `+axis` in both orders, so the
  broadphase (4d) must emit each pair once, in a stable order.

### Broadphase

- `sweep_and_prune` sorts proxies by (min.x, entity key), then scans forward while
  `min.x <= max.x` of the current proxy and confirms with the full 3-axis `overlaps` (touching
  counts). Fixed x axis; the result never depends on the axis.
- Each pair is emitted once, oriented (lower key, higher key) with key = (index, generation), and
  the output is sorted lexicographically. The result is identical for any input permutation, which
  matters because degenerate narrowphase cases are not antisymmetric.
- `brute_force_pairs` is the O(n^2) reference with the same contract (tests, 4f benchmark).
- `skip_static_pairs` drops pairs where both proxies are static (default off).
- Preconditions: unique entities, no NaN bounds (Debug asserts; Release ignores NaN proxies).

### Solver and PhysicsWorld (`solver.hpp`, `world.hpp`)

`PhysicsWorld::step(registry, dt)` runs inside one `FixedTimestep` tick, in this order:
1. `store_previous_transforms`, 2. `integrate_bodies` (gravity from `PhysicsSettings`),
3. broadphase (`sweep_and_prune`, `skip_static_pairs = true`), 4. per pair, in broadphase order:
   narrowphase, record in `contacts()`, build a constraint unless a trigger is involved,
5. `solve_contacts`.

- Triggers: `Collider::is_trigger`. The trigger check comes *before* the "no dynamic body"
  discard, so a kinematic body (or a kinematic-static pair) is reported inside a trigger.
  Static-static pairs never reach the narrowphase (broadphase filter), so a static body is not
  reported against a static trigger. Trigger pairs are never resolved.
- Pairs without a dynamic body (static-static, kinematic-static, kinematic-kinematic) that
  involve no trigger are discarded before the narrowphase: no response, not in `contacts()`.
- `contacts()` holds solid and trigger overlaps of the last step (`ContactPair::trigger`), in the
  canonical broadphase order, normal from `a` to `b`. No `EventBus`: callers poll each step.
- Solver: sequential impulses, `velocity_iterations` passes (default 10) over the constraints in
  a fixed order, normal impulse (accumulated, >= 0) then Coulomb friction (3D tangent vector,
  clamped to `mu * Pn`, no tangent basis). No warm starting. Mixing: `mu = sqrt(mu_a * mu_b)`,
  `e = max(e_a, e_b)`. Restitution only above `restitution_threshold` (1 m/s closing speed), so
  resting contacts do not jitter.
- Position correction: after the velocity passes, one direct projection by
  `max(depth - slop, 0) * correction_percent` (slop 0.01, percent 0.4), split by inverse mass. It
  does not touch velocities, so it adds no energy. The depth is the one measured before solving,
  hence `percent < 1`. Resting depth is about `slop + g*dt^2*(1/percent - 1)` (1.4 cm at 60 Hz).
- One contact point per pair is exact: with no rotation there is no torque, so every point of a
  box-box manifold would have the same Jacobian.
- Determinism: pair order is canonical, constraints and iterations have a fixed order, no hashing
  or pointer ordering. Bit-for-bit repeatable on the same build and platform; across compilers
  it is not guaranteed (e.g. FMA contraction).
- Known limits: no CCD (fast or thin bodies can tunnel), no warm starting (tall stacks or large
  mass ratios converge slowly), no sleeping, no rotation, no layers.
