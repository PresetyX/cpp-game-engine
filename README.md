# cpp-game-engine

[![CI](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml)

A C++17 / OpenGL 3.3 game engine, built step by step as a portfolio project.

**Status: early development.** Implemented and unit-tested so far: a logger, a frame timer,
a fixed-timestep accumulator with render interpolation, and a typed event bus (`engine/core`),
plus an ECS with generational entity ids, sparse-set component storage and views
(`engine/ecs`, benchmark numbers in [docs/benchmarks.md](docs/benchmarks.md)).
Also: `engine/math` (GLM) and `engine/physics`, a 3D translation-only physics (spheres and
axis-aligned boxes, sweep-and-prune broadphase, sequential-impulse solver with friction, triggers);
5k bodies step in ~3 ms (see [docs/benchmarks.md](docs/benchmarks.md)).
`engine/assets` loads images (stb_image), OBJ meshes and GLSL source text into CPU memory
through a cached `AssetManager` with hot reload. `engine/render` is an OpenGL 3.3 core renderer
(GLFW window, Blinn-Phong with one directional and up to 8 point lights, orbit camera, hot reload
of shaders, meshes and textures, interpolated drawing). Two samples use it: `physics_playground`
and `model_viewer` (see [Samples](#samples)).

## Build and test

Requirements: CMake >= 3.20, a C++17 compiler (GCC, Clang or MSVC) and Git.
Dependencies are fetched automatically with CMake `FetchContent`.

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To build the samples (`ENGINE_BUILD_SAMPLES`, OFF by default; running them needs an OpenGL 3.3
driver; on Linux, `xorg-dev` and `libgl-dev` are needed to compile GLFW):

```sh
cmake -B build -DENGINE_BUILD_SAMPLES=ON && cmake --build build
./build/samples/physics_playground/physics_playground
./build/samples/model_viewer/model_viewer [path/to/model.obj]
```

## Samples

Both open a 1280x720 window, use the same Blinn-Phong renderer and share the assets in
[samples/assets](samples/assets/README.md) (origin and license of each file are listed there).
Mouse and keyboard handling is sample code (`samples/common`); the engine has no input module.

### physics_playground

Spheres and boxes fall and collide on a static floor. 40 bodies at the start, at most 300.
The window title shows the number of bodies and the physics step time (mean over the last
0.5 s, measured around `PhysicsWorld::step`).

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
command line and can be anywhere on disk (an absolute key bypasses the manager root); with no
argument it shows the bundled `samples/assets/models/torus.obj`. The camera starts framed on the
model's bounding sphere. The title shows the file name and the triangle count.

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

## What was tested

- Unit tests (`ctest`, 265 in Debug / 266 in Release on MSVC) cover core, ECS, math, physics,
  assets and the GPU-free part of the render module, including that the bundled sample model
  loads and that `phong.frag` and the C++ light limits agree.
- The samples were started on Windows 11 with an NVIDIA GPU (OpenGL 3.3 core): both open, the
  physics title updates, Space adds a body, R resets, and a deliberately broken `phong.frag` is
  logged with file and line while the old shader keeps running, then reloads when fixed. Mouse
  orbit and wheel zoom were not exercised by a script.

## Limitations

- No sRGB: textures and the framebuffer are linear, so lighting looks harsher than gamma-correct
  output.
- Linux is only compiled, never run (CI: GCC and Clang on Ubuntu, samples built with
  `ENGINE_BUILD_SAMPLES=ON`, not executed). macOS is not built anywhere. The samples were run on
  Windows only.
- No test opens a window: the `engine_render` module (window, GL objects, `Renderer`) is covered
  by the manual run above, not by automated tests (`ENGINE_BUILD_GPU_TESTS` is declared but empty).
- No shadows, PBR, instancing or frustum culling; one draw call per entity, at most 8 point lights.
- OBJ: no materials (`mtllib` is ignored); `model_viewer` draws one flat color.
- Physics is translation-only (no rotation), spheres and axis-aligned boxes. See
  [docs/architecture.md](docs/architecture.md) for the limits of each module.

## Roadmap

- [x] 1. Project skeleton and CI
- [x] 2. Core (logger, fixed-timestep loop, events; simple allocators still pending)
- [x] 3. ECS
- [x] 4. Math and physics
- [x] 5. Assets
- [x] 6. Render
- [x] 7. Samples
- [ ] 8. Quality (clang-tidy, sanitizers, coverage)
- [ ] 9. Documentation
