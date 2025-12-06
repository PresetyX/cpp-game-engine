# Third-party dependencies, fetched on demand and pinned by tag.
# Only dependencies used by existing modules are listed; the rest are added in
# the step that needs them (GLFW, glad, GLM, stb_image, spdlog).
include(FetchContent)

if(ENGINE_BUILD_TESTS)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()
