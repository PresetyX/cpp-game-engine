# Progress

## Completed steps
- [x] 1. Skeleton and CI
- [x] 2. Core: logger, clock/FrameTimer, FixedTimestep, EventBus (31 tests, MSVC Debug+Release)
- [x] 3. ECS: EntityPool (generational ids), SparseSet, Registry, View + benchmark
  (78 tests total, MSVC Debug+Release; design by the `arquiteto` agent)
- [ ] 4. Math and 3D physics (plan by `arquiteto`)
  - [x] 4a. GLM 1.0.1 + `engine_math` (`Transform`, `PreviousTransform`, `interpolate`, `Aabb`);
    94 tests total, MSVC Debug+Release
  - [x] 4b. `engine_physics`: `Collider`, `RigidBody`, `store_previous_transforms`,
    `integrate_bodies` (gravity, forces, damping, semi-implicit Euler); 106 tests total, MSVC
    Debug+Release
  - [x] 4c. Narrowphase: `Contact`, `collide_sphere_sphere/sphere_box/box_box`, `collide`
    (design by `arquiteto`); 138 tests total, MSVC Debug+Release
  - [ ] 4d. Broadphase sweep-and-prune (vs brute force in seeded random scenes)
  - [ ] 4e. Solver (sequential impulses, restitution, Coulomb friction) + `PhysicsWorld`
  - [ ] 4f. Benchmark (1k/5k bodies) + docs

## Decisions
- Only GoogleTest is fetched in step 1; GLFW/glad/GLM/stb_image/spdlog are added in the steps
  that use them (avoids X11 packages in CI before step 6).
- Local validation is done on Windows with MSVC (VS Build Tools 2022, Ninja); CI covers
  GCC and Clang on Linux. Step 1 verified locally with MSVC `/W4 /WX`, Debug and Release.

- Step 2: own logger instead of spdlog; `FixedTimestep` is a pure accumulator (no clock) with a
  0.25 s frame clamp; `EventBus` supports immediate `publish` and deferred `enqueue`/`flush`.
  Details in `docs/architecture.md`.
- Step 3: 32-bit index + 32-bit generation (no bit packing); one non-paged sparse set per
  component type; views iterate the smallest pool backwards so destroying the current entity
  is safe; `ENGINE_BUILD_BENCHMARKS` (OFF by default, built but not run in CI). Details in
  `docs/architecture.md`. Measured on Ryzen 7 9800X3D / MSVC Release: ~3.5 ns/entity for the
  Position+Velocity update, see `docs/benchmarks.md`.
- Step 4 (decided with the user): sample is 3D, so physics is 3D translation-only with sphere +
  AABB shapes; GLM is used; simple Coulomb friction (one coefficient per collider); no events,
  `contacts()` per step only. Static body = entity with `Collider` and no `RigidBody`. GLM include
  dirs are marked SYSTEM in `cmake/Dependencies.cmake`; `engine_math` is an INTERFACE library, so
  `engine_set_warnings` is not applied to it (the headers are compiled with warnings via tests).

## Commands
Linux:
```sh
cmake -B build -G Ninja && cmake --build build && ctest --test-dir build --output-on-failure
```
Windows (from a Developer PowerShell, or after running `vcvars64.bat`):
```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ctest --test-dir build --output-on-failure
```

## Pending
- Simple allocators (listed under `core/` in the target structure) are not implemented yet.
- Linux CI (GCC/Clang) has not run yet for steps 4a/4b/4c; check the Actions result after the push.
- Generation-exhaustion slot retirement in `EntityPool` is untested (needs 2^32 cycles).
- Benchmark was run on Windows/MSVC only; Linux numbers not measured.
- The 4d broadphase must emit each pair once in a stable order (degenerate narrowphase cases are
  not antisymmetric, see `docs/architecture.md`).
- Next: step 4d.
