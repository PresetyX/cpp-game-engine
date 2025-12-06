# Architecture

Work in progress; sections are added as modules are implemented.

## Decisions

- **Modules as static libraries** (`engine_core`, later `engine_ecs`, `engine_physics`, ...),
  each with an `engine::<name>` alias. GPU-free logic stays in libraries that tests can link
  without creating a window.
- **Dependencies on demand.** `cmake/Dependencies.cmake` only fetches what existing modules use
  (currently GoogleTest). Each library is pinned by tag.
- **Warnings policy.** `-Wall -Wextra -Wpedantic -Werror` (MSVC: `/W4 /WX`) applied via
  `engine_set_warnings()` to our own targets only.
