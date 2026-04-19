#pragma once

#include <memory>
#include <string>

#include "engine/assets/result.hpp"

struct GLFWwindow;

namespace engine::render {

struct WindowConfig {
    int width = 1280;
    int height = 720;
    std::string title = "engine";
    bool vsync = true;
};

struct FramebufferSize {
    int width = 0;
    int height = 0;
    // A minimized window reports 0; callers must skip rendering then.
    bool empty() const { return width <= 0 || height <= 0; }
};

// RAII GLFW window with an OpenGL 3.3 core context, made current on the creating thread.
// GLFW is initialized by the first Window and terminated when the last one is destroyed.
// Destroy every GL object (Renderer, meshes, textures) before the Window.
class Window {
public:
    static assets::LoadResult<std::unique_ptr<Window>> create(const WindowConfig& config);

    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool should_close() const;
    void request_close();
    void poll_events();
    void swap_buffers();

    // Size in pixels (not screen coordinates), updated by the resize callback.
    FramebufferSize framebuffer_size() const { return size_; }
    double time() const;  // seconds since GLFW init

    GLFWwindow* handle() const { return window_; }

private:
    Window() = default;

    GLFWwindow* window_ = nullptr;
    FramebufferSize size_;
};

}  // namespace engine::render
