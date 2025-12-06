# cpp-game-engine

[![CI](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml)

A C++17 / OpenGL 3.3 game engine, built step by step as a portfolio project.

**Status: early development.** Implemented and unit-tested so far: a logger, a frame timer,
a fixed-timestep accumulator with render interpolation, and a typed event bus (`engine/core`).
There is no rendering, ECS, physics or asset code yet.

## Build and test

Requirements: CMake >= 3.20, a C++17 compiler (GCC, Clang or MSVC) and Git.
Dependencies are fetched automatically with CMake `FetchContent`.

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Roadmap

- [x] 1. Project skeleton and CI
- [x] 2. Core (logger, fixed-timestep loop, events; simple allocators still pending)
- [ ] 3. ECS
- [ ] 4. Math and physics
- [ ] 5. Assets
- [ ] 6. Render
- [ ] 7. Samples
- [ ] 8. Quality (clang-tidy, sanitizers, coverage)
- [ ] 9. Documentation
