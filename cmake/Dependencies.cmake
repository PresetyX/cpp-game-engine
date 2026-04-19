# Third-party dependencies, fetched on demand and pinned by tag.
# Only dependencies used by existing modules are listed; the rest are added in
# the step that needs them (GLFW, glad, stb_image, spdlog).
include(FetchContent)

# GLM (header-only math). Fetched unconditionally: engine_math depends on it.
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glm)

# Mark GLM's include directories as SYSTEM so its warnings do not trip
# /W4 /WX or -Wpedantic -Werror. (FetchContent's SYSTEM option needs CMake 3.25.)
set(_glm_target glm)
if(TARGET glm::glm)
    get_target_property(_glm_aliased glm::glm ALIASED_TARGET)
    if(_glm_aliased)
        set(_glm_target ${_glm_aliased})
    endif()
endif()
get_target_property(_glm_includes ${_glm_target} INTERFACE_INCLUDE_DIRECTORIES)
if(_glm_includes)
    set_target_properties(${_glm_target} PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_glm_includes}")
endif()

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

# stb_image (single header, public domain), pinned by commit and checked by SHA-256.
# Only the header is downloaded; the implementation is compiled in engine_assets.
FetchContent_Declare(stb_image
    URL https://raw.githubusercontent.com/nothings/stb/6b5799bae8a1fe3792691ef4b72ae26434c3e799/stb_image.h
    URL_HASH SHA256=dfc2aadda6295f67f677587773f25e5bf8c8ad427ed0db7c518525c91e72e020
    DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_MakeAvailable(stb_image)
set(STB_IMAGE_DIR "${stb_image_SOURCE_DIR}" CACHE INTERNAL "")
