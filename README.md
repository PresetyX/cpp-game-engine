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
of shaders, meshes and textures, interpolated drawing); the scene sample shows it.

## Build and test

Requirements: CMake >= 3.20, a C++17 compiler (GCC, Clang or MSVC) and Git.
Dependencies are fetched automatically with CMake `FetchContent`.

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To build and run the render sample (needs an OpenGL 3.3 driver; on Linux, `xorg-dev` and
`libgl-dev` to compile GLFW):

```sh
cmake -B build -DENGINE_BUILD_SAMPLES=ON && cmake --build build
./build/samples/scene/scene_sample
```

## Roadmap

- [x] 1. Project skeleton and CI
- [x] 2. Core (logger, fixed-timestep loop, events; simple allocators still pending)
- [x] 3. ECS
- [x] 4. Math and physics
- [x] 5. Assets
- [x] 6. Render
- [ ] 7. Samples
- [ ] 8. Quality (clang-tidy, sanitizers, coverage)
- [ ] 9. Documentation
