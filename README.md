# cpp-game-engine

[![CI](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml)

A C++17 / OpenGL 3.3 game engine, built step by step as a portfolio project.

**Status: early development.** At the moment the repository only contains the build
system, CI and an empty `core` library. No engine features are implemented yet.

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
- [ ] 2. Core (logger, fixed-timestep loop, events)
- [ ] 3. ECS
- [ ] 4. Math and physics
- [ ] 5. Assets
- [ ] 6. Render
- [ ] 7. Samples
- [ ] 8. Quality (clang-tidy, sanitizers, coverage)
- [ ] 9. Documentation
