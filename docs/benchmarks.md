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

## Physics: 1k and 5k bodies

Program: [`benchmarks/physics_bench.cpp`](../benchmarks/physics_bench.cpp), timed with
`std::chrono::steady_clock`.

Scene: a static floor and four static walls around a 20 x 20 grid; N dynamic bodies (alternating
spheres of radius 0.5 and boxes of half extent 0.5, mass 1, friction 0.5, no restitution) start in
layers of 400 (spacing 1.05, 1.5 between layers; 1k = 2.5 layers, 5k = 12.5 layers) and fall onto
the floor. Default `PhysicsSettings` (10 velocity iterations), `dt = 1/60`.

- **PhysicsWorld::step**: 300 steps (5 simulated seconds: fall, impact, settle) on a fresh scene,
  3 runs, all 900 samples pooled. It covers the whole pipeline (previous transforms, integration,
  broadphase, narrowphase, solver, position correction). The min is the free-fall phase with no
  contacts; the median and max include the pile.
- **sweep_and_prune / brute_force_pairs**: both run on the proxies of the settled scene
  (`skip_static_pairs = true`), 21 timed runs each. The program checks that both return the same
  number of pairs. The proxy build is not timed.

No bodies sleep and there is no warm starting, so every body costs the same each step.

### How to run (Release)

```powershell
cmake -B build-bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DENGINE_BUILD_BENCHMARKS=ON -DENGINE_BUILD_TESTS=OFF
cmake --build build-bench --target physics_bench
build-bench\benchmarks\physics_bench.exe
```

### Machine

Same machine, OS, compiler, flags and power plan as the ECS table above. Date 2026-11-23.
Single execution; no CPU pinning; other desktop applications were open.

### Results

| Bodies | Scenario | Samples | Min (ms) | Median (ms) | Max (ms) |
|---|---|---|---|---|---|
| 1000 | PhysicsWorld::step | 900 | 0.081 | 0.464 | 0.591 |
| 1000 | sweep_and_prune (settled) | 21 | 0.072 | 0.073 | 0.090 |
| 1000 | brute_force_pairs (settled) | 21 | 0.337 | 0.338 | 0.355 |
| 5000 | PhysicsWorld::step | 900 | 1.071 | 2.937 | 3.680 |
| 5000 | sweep_and_prune (settled) | 21 | 1.239 | 1.287 | 1.587 |
| 5000 | brute_force_pairs (settled) | 21 | 8.126 | 8.331 | 12.411 |

After the last step the scene had 1000 contacts (1k bodies) and 5800 contacts (5k bodies); the
broadphase returned the same numbers of pairs.

Reading: the median step is 0.46 ms (1k) and 2.9 ms (5k), well inside a 16.7 ms tick at 60 Hz.
From 1k to 5k bodies the step grows ~6.3x for 5x the bodies. In this scene sweep-and-prune is
~4.6x (1k) and ~6.5x (5k) faster than brute force. This is a mostly flat, evenly spaced layout
with a fixed x axis; other layouts (e.g. many bodies lined up along x) were not measured. The
split of the step time between narrowphase and solver was not measured.
