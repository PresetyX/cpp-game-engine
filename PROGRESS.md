# Progress

## Completed steps
- [x] 1. Skeleton and CI
- [x] 2. Core: logger, clock/FrameTimer, FixedTimestep, EventBus (31 tests, MSVC Debug+Release)
- [x] 3. ECS: EntityPool (generational ids), SparseSet, Registry, View + benchmark
  (78 tests total, MSVC Debug+Release; design by the `arquiteto` agent)

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
- Linux CI (GCC/Clang) has not run yet; commits are local until pushed.
- Generation-exhaustion slot retirement in `EntityPool` is untested (needs 2^32 cycles).
- Benchmark was run on Windows/MSVC only; Linux numbers not measured.
- Next: step 4 (math and physics), to be delegated to the `arquiteto` agent.
