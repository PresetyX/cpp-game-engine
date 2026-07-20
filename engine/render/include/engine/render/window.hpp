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

namespace detail {
// State written by GLFW callbacks.
struct WindowState {
    FramebufferSize size;
    double scroll = 0.0;
};
}  // namespace detail

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
    FramebufferSize framebuffer_size() const { return state_.size; }

    // Minimal input for samples (the engine has no input module yet). Key and button codes are
    // the GLFW ones (GLFW_KEY_ESCAPE, GLFW_MOUSE_BUTTON_LEFT, ...).
    bool key_down(int key) const;
    bool mouse_button_down(int button) const;
    // Cursor position in screen coordinates, origin at the top-left corner.
    void cursor_position(double& x, double& y) const;
    // Scroll offset accumulated since the last call (vertical wheel steps); resets to 0.
    double consume_scroll();
    double time() const;  // seconds since GLFW init

    GLFWwindow* handle() const { return window_; }

private:
    Window() = default;

    // Its address is the GLFW user pointer, so the Window must not move (it is heap-allocated).
    GLFWwindow* window_ = nullptr;
    detail::WindowState state_;
};

}  // namespace engine::render
