# Benchmarks

Every number here was measured on the machine listed below; none are estimates.
Re-run after changes to `engine/ecs`. Results vary between runs and machines, so
compare rows from the same run only.

## ECS: 100k entities

Program: [`benchmarks/ecs_bench.cpp`](../benchmarks/ecs_bench.cpp), timed with
`std::chrono::steady_clock`. Each entity has `Position` and `Velocity` (3 floats each).

- **create**: create 100k entities and add both components (15 runs, fresh registry each run).
- **update**: `view<Position, const Velocity>().each(p += v * dt)` over the 100k entities
  (5 warm-up passes, then 101 timed passes).
- **destroy**: destroy all 100k entities, which also removes their 2 components (15 runs).

No baseline against plain arrays was measured, so nothing here claims how this compares to
other approaches.

### How to run (Release)

```powershell
cmake -B build-bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DENGINE_BUILD_BENCHMARKS=ON -DENGINE_BUILD_TESTS=OFF
cmake --build build-bench --target ecs_bench
build-bench\benchmarks\ecs_bench.exe
```

On Linux the binary is `build-bench/benchmarks/ecs_bench`.

### Machine

| Field | Value |
|---|---|
| Date | 2026-11-23 |
| Base commit | `097ec90` (plus the step 3 changes, not yet committed when measured) |
| CPU | AMD Ryzen 7 9800X3D, 8 cores / 16 threads |
| RAM | 31.1 GB |
| OS | Windows 11 Pro 10.0.26200 |
| Compiler | MSVC 19.44.35230 (VS Build Tools 2022) |
| Build flags | Release, `/O2 /Ob2 /DNDEBUG` (CMake default) |
| Power plan | "FSOS AMD Gaming" |

### Results

Single execution of the program; no CPU pinning; other desktop applications were open.

| Scenario (100k entities) | Runs | Min (ms) | Median (ms) | Median ns/entity |
|---|---|---|---|---|
| create + add Position + add Velocity | 15 | 6.711 | 7.012 | 70.1 |
| view<Position, const Velocity>().each | 101 | 0.348 | 0.350 | 3.5 |
| destroy (removes 2 components each) | 15 | 0.847 | 0.895 | 9.0 |

Checksum printed by the program: `5.001e+09` (keeps the compiler from discarding the update).
