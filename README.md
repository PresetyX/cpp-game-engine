# cpp-game-engine

[![CI](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml)

A small C++17 / OpenGL 3.3 game engine, built step by step as a portfolio project: an ECS, a 3D
rigid-body physics (translation only), an asset manager with hot reload, and a Blinn-Phong
renderer, with two runnable samples. Nothing is hidden behind a framework: the modules are small
static libraries ([architecture and dependency diagram](docs/architecture.md)).

| Module | What it has |
|---|---|
| `engine/core` | logger, frame clock, `FixedTimestep` with render interpolation, typed `EventBus` |
| `engine/ecs` | generational entity ids, sparse-set storage, `Registry`, views |
| `engine/math` | GLM 1.0.1 plus `Transform`, `PreviousTransform`, `interpolate`, `Aabb` |
| `engine/physics` | spheres and axis-aligned boxes, sweep-and-prune broadphase, sequential-impulse solver with restitution and Coulomb friction, triggers |
| `engine/assets` | `AssetManager` (handles, ref counts, polling hot reload); images (stb_image), OBJ meshes, GLSL source |
| `engine/render` | GLFW window, shaders, meshes, textures, Blinn-Phong `Renderer` (one directional + up to 8 point lights), orbit camera, interpolated drawing, hot reload of shaders/meshes/textures |

## Build and test

Requirements: CMake >= 3.20, a C++17 compiler (GCC, Clang or MSVC) and Git. Dependencies are
fetched with CMake `FetchContent` (GoogleTest, GLM, GLFW, stb_image); glad is committed.

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows, run it from a Developer PowerShell (or after `vcvars64.bat`); the project was
developed with MSVC (VS Build Tools 2022) and Ninja. On Linux, compiling GLFW needs `xorg-dev` and
`libgl-dev`.

| CMake option | Default | Meaning |
|---|---|---|
| `ENGINE_BUILD_TESTS` | ON | unit tests (`engine_tests`) |
| `ENGINE_BUILD_SAMPLES` | OFF | the two samples (running them needs an OpenGL 3.3 driver) |
| `ENGINE_BUILD_BENCHMARKS` | OFF | `ecs_bench`, `physics_bench` (not run by ctest) |
| `ENGINE_WARNINGS_AS_ERRORS` | ON | `-Werror` / `/WX` on engine targets |
| `ENGINE_SANITIZE` | OFF | AddressSanitizer + UBSan (GCC/Clang) |
| `ENGINE_COVERAGE` | OFF | gcov instrumentation (GCC/Clang) |
| `ENGINE_BUILD_GPU_TESTS` | OFF | declared for tests that need a GL context; there are none yet |

## Samples

```sh
cmake -B build -DENGINE_BUILD_SAMPLES=ON && cmake --build build
./build/samples/physics_playground/physics_playground
./build/samples/model_viewer/model_viewer [path/to/model.obj]
```

Both open a 1280x720 window, use the same renderer and share the assets in
[samples/assets](samples/assets/README.md) (origin and license of every file are listed there).
Mouse and keyboard handling is sample code (`samples/common`); the engine has no input module.

### physics_playground

Spheres and boxes fall and collide on a static floor. 40 bodies at the start, at most 300. The
window title shows the number of bodies and the physics step time (mean over the last 0.5 s,
measured around `PhysicsWorld::step`).

| Input | Action |
|---|---|
| Left mouse drag | orbit the camera |
| Mouse wheel | zoom |
| Space | drop a new body (alternates sphere and box) |
| R | reset the scene to its initial 40 bodies |
| Esc | quit |

> **Screenshot / GIF: to be added** as `docs/physics_playground.gif` (not recorded yet).

### model_viewer

Loads one OBJ through the `AssetManager` and shows it with three lights. The path is given on the
command line and can be anywhere on disk; with no argument it shows the bundled
`samples/assets/models/torus.obj`. The camera starts framed on the model's bounding sphere. The
title shows the file name and the triangle count.

| Input | Action |
|---|---|
| Left mouse drag | orbit the camera |
| Mouse wheel | zoom |
| R | frame the model again |
| Esc | quit |

Hot reload: while it runs, edit `samples/assets/shaders/phong.frag` (or `.vert`, or the OBJ) and
save. Within about a second the change appears; a shader that does not compile is logged with
file and line and the previous one stays active. The samples read the shaders from the source
folder, not from a copy.

> **Screenshot / GIF: to be added** as `docs/model_viewer.gif` (not recorded yet).

## What is tested

Every number below was measured; the commands to reproduce it are given. CI is GitHub Actions on
Ubuntu (GCC and Clang, Debug and Release); the local machine is Windows 11 with MSVC.

| What | Result | Where / how |
|---|---|---|
| Unit tests | 265 (Debug) / 266 (Release) pass, MSVC `/W4 /WX` and on Linux CI with GCC and Clang | `ctest --test-dir build`; the sources are in `tests/` |
| ASan + UBSan | the 265 Debug tests pass, no report (Clang, Linux CI) | `cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DENGINE_SANITIZE=ON`, then build and `ctest`; CI job `clang / ASan + UBSan` |
| Line coverage | **97.3 %** (1232 of 1266 lines), functions 93.5 % (709 of 758), 38 files of `engine/`, Debug, GCC 15.2.0, lcov 2.4 (CI run on commit `215c04d`, 2026-11-23) | see below |
| clang-tidy | 0 warnings on `engine/*/src`, `samples/*/main.cpp` and the headers of `engine/` and `samples/` (LLVM 19.1.5); three checks are disabled on purpose, with the reason in `.clang-tidy` | `clang-tidy -p build-Release $(ls engine/*/src/*.cpp samples/*/main.cpp)` |
| Benchmarks (Windows, MSVC Release, Ryzen 7 9800X3D) | ECS update 3.5 ns/entity (100k entities); physics step median 0.46 ms (1k bodies) / 2.9 ms (5k bodies) | [docs/benchmarks.md](docs/benchmarks.md), with the machine and the commands |
| Samples (run by hand once, Windows 11, NVIDIA GPU, OpenGL 3.3 core) | both start; the physics title updates; Space adds a body; R resets; a deliberately broken `phong.frag` is logged with file and line while the old shader keeps running, and the fixed file reloads; replacing the OBJ of a running `model_viewer` reloads it (title went from 1024 to 12 triangles; the redraw itself was not inspected) | not automated; mouse orbit and wheel zoom were not exercised by a script |

Coverage, as computed in CI (on Linux; the job is `gcc / line coverage` in
[ci.yml](.github/workflows/ci.yml)):

```sh
cmake -B build-cov -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENGINE_COVERAGE=ON
cmake --build build-cov && ctest --test-dir build-cov
IGN="--ignore-errors mismatch,inconsistent,unused,negative,count --rc branch_coverage=0"
lcov --capture --directory build-cov --output-file all.info $IGN
lcov --extract all.info "$PWD/engine/*" --output-file engine.info $IGN
lcov --summary engine.info $IGN
```

What the 97.3 % does and does not cover: it counts the `engine/` files that the test binary
links. `engine_render` (`Window`, GL objects, `Renderer`) is not linked into the tests, so it is
not in the figure and its coverage is effectively zero; that is the "no test opens a window"
limitation below. The render logic that is testable without a GPU (cameras, draw list,
light packing, shader log parsing, `UploadCache`) is in the figure. The `--ignore-errors`
list is there because GCC 15 and GoogleTest produce line records that lcov 2.x flags as
"inconsistent"; lcov reports them as warnings and the totals above are computed with them ignored.
The figure is line coverage; branch coverage was not measured.

## Design decisions

The longer reasoning, with the alternatives that were rejected, is in
[docs/architecture.md](docs/architecture.md). The short version:

- **Modules are static libraries, and GPU-free logic never links the renderer.** `engine_tests`
  does not link `render`, so no test opens a window. The price is that the GL code is only
  checked by hand (see the limitations below).
- **Fixed timestep driven by an accumulator that never reads a clock.** `FixedTimestep` receives
  the frame time from the caller, so tests can replay exact sequences. Frame time is clamped
  (0.25 s by default) so a long stall slows the simulation instead of causing a catch-up spiral;
  rendering interpolates with `alpha()`.
- **ECS on sparse sets, not archetypes.** Add and remove are O(1) and there are no table moves,
  at the cost of multi-component views checking membership against the smallest pool and of one
  entity's components not being adjacent in memory.
- **Generational 32 + 32 bit entity ids.** A packed 20/12 bit handle was rejected because 4096
  reuses of a slot would wrap the generation and revive stale handles.
- **Physics is translation only, solved with sequential impulses.** Without rotation there is no
  torque, so one contact point per pair is exact. The solver has no warm starting (10 fixed
  iterations), which is why tall stacks are not rigid. Pair order is canonical, so a run is
  repeatable bit for bit on the same build and platform, but not guaranteed across compilers.
- **Position correction is a direct projection that does not touch velocities,** so it adds no
  energy; restitution applies only above a closing speed of 1 m/s so resting contacts do not
  jitter.
- **Dependencies on demand, pinned.** Only what existing modules use is fetched, each pinned by
  tag or by URL plus SHA-256. The logger is in-house to avoid one more dependency.
- **Hot reload keyed on asset version.** The renderer re-uploads only what has a different
  version, and a shader that fails to recompile is logged with file and line while the old
  program keeps running, with no retry until the file changes.
- **Limits that exist in two places are tested against each other.** `kMaxPointLights` is
  duplicated in `phong.frag`, and a test reads the shader and fails if the two differ.

## Known limitations

- **No sRGB.** Textures and the framebuffer are linear, so lighting is not gamma-correct.
- **Physics is translation only: no rotation.** Boxes are axis-aligned in world space.
- **No continuous collision detection (CCD).** Fast or thin bodies can tunnel through others; there
  is no test that measures it, it is simply not implemented.
- **Tall stacks are not rigid.** The solver has no warm starting and does a fixed 10 iterations. A
  stack of 3 boxes settles with the bottom box about 2.9 cm into the floor and a residual vertical
  speed up to about 1 cm/s (measured in `World.SmallBoxStackIsStable`); taller stacks were not
  measured and converge more slowly. No sleeping.
- **Linux and macOS: the samples never run.** The unit tests run on Linux in CI, and the samples are
  compiled there (`ENGINE_BUILD_SAMPLES=ON`), but nothing has executed them. macOS is not built
  anywhere. The samples were run on Windows with one NVIDIA GPU only; Mesa and Intel/AMD shader
  log formats are covered only by text fixtures.
- **No test opens a window.** `engine_render` has no automated test (`ENGINE_BUILD_GPU_TESTS` is
  declared but empty); it was checked by hand as described above.
- Rendering: no shadows, PBR, instancing or frustum culling; one draw call per entity; at most 8
  point lights. OBJ: materials (`mtllib`) are ignored, so `model_viewer` draws one flat color.
- Assets are loaded synchronously and hot reload polls file times (no file-watch events).
- Not implemented: simple allocators (planned under `core/`), a real input module, Wayland.
- `EntityPool` slot retirement after 2^32 generations is untested (it would need 2^32 cycles).
- Benchmarks were measured on Windows/MSVC only.

## Roadmap

Done (details in [PROGRESS.md](PROGRESS.md) and [docs/architecture.md](docs/architecture.md)):

- [x] 1. Project skeleton and CI
- [x] 2. Core (logger, fixed-timestep loop, events)
- [x] 3. ECS
- [x] 4. Math and physics
- [x] 5. Assets
- [x] 6. Render
- [x] 7. Samples
- [x] 8. Quality (clang-tidy, sanitizers, coverage)
- [x] 9. Documentation

Not planned yet, in no particular order: rotation and a box-box manifold with torque, warm
starting and sleeping, CCD, sRGB, GPU tests with a headless context, simple allocators, GIFs of
the samples (`docs/*.gif`).

## Development history

This project was developed locally between 2025-11-26 and 2026-11-23. The commit history was created afterwards, and the commit dates were adjusted to match the period in which the work was actually done.

## License

MIT, see [LICENSE](LICENSE). The sample assets are listed in
[samples/assets/README.md](samples/assets/README.md).
