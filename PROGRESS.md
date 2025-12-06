# Progress

## Completed steps
- [x] 1. Skeleton and CI

## Decisions
- Only GoogleTest is fetched in step 1; GLFW/glad/GLM/stb_image/spdlog are added in the steps
  that use them (avoids X11 packages in CI before step 6).
- Local validation is done on Windows with MSVC (VS Build Tools 2022, Ninja); CI covers
  GCC and Clang on Linux. Step 1 verified locally with MSVC `/W4 /WX`, Debug and Release.

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
- Next: step 2 (Core: logger, clock, fixed-timestep loop, typed events, accumulator tests).
