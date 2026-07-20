#include "engine/render/window.hpp"

#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include "engine/core/log.hpp"

namespace engine::render {

namespace {

int g_window_count = 0;
std::string g_glfw_error;

void on_glfw_error(int code, const char* description) {
    g_glfw_error = std::string(description != nullptr ? description : "unknown") + " (code " +
                   std::to_string(code) + ")";
    log_error("GLFW: ", g_glfw_error);
}

void on_framebuffer_size(GLFWwindow* handle, int width, int height) {
    auto* self = static_cast<detail::WindowState*>(glfwGetWindowUserPointer(handle));
    if (self != nullptr) {
        self->size.width = width;
        self->size.height = height;
    }
}

void on_scroll(GLFWwindow* handle, double /*x*/, double y) {
    auto* self = static_cast<detail::WindowState*>(glfwGetWindowUserPointer(handle));
    if (self != nullptr) {
        self->scroll += y;
    }
}

}  // namespace

assets::LoadResult<std::unique_ptr<Window>> Window::create(const WindowConfig& config) {
    using Result = assets::LoadResult<std::unique_ptr<Window>>;

    if (g_window_count == 0) {
        glfwSetErrorCallback(on_glfw_error);
        if (glfwInit() != GLFW_TRUE) {
            return Result::failure("glfwInit failed: " + g_glfw_error);
        }
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // required on macOS, harmless elsewhere
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* handle =
        glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (handle == nullptr) {
        const std::string reason = g_glfw_error;
        if (g_window_count == 0) {
            glfwTerminate();
        }
        return Result::failure("could not create an OpenGL 3.3 core window: " + reason);
    }

    glfwMakeContextCurrent(handle);
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        glfwDestroyWindow(handle);
        if (g_window_count == 0) {
            glfwTerminate();
        }
        return Result::failure("gladLoadGL failed: the driver does not provide OpenGL 3.3");
    }
    glfwSwapInterval(config.vsync ? 1 : 0);

    std::unique_ptr<Window> window(new Window());
    window->window_ = handle;
    glfwGetFramebufferSize(handle, &window->state_.size.width, &window->state_.size.height);
    glfwSetWindowUserPointer(handle, &window->state_);
    glfwSetFramebufferSizeCallback(handle, on_framebuffer_size);
    glfwSetScrollCallback(handle, on_scroll);
    ++g_window_count;

    log_info("OpenGL ", reinterpret_cast<const char*>(glGetString(GL_VERSION)), " / ",
             reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    return window;
}

Window::~Window() {
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        if (--g_window_count == 0) {
            glfwTerminate();
        }
    }
}

bool Window::should_close() const {
    return glfwWindowShouldClose(window_) == GLFW_TRUE;
}

void Window::request_close() {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Window::poll_events() {
    glfwPollEvents();
}

void Window::swap_buffers() {
    glfwSwapBuffers(window_);
}

bool Window::key_down(int key) const {
    return glfwGetKey(window_, key) == GLFW_PRESS;
}

bool Window::mouse_button_down(int button) const {
    return glfwGetMouseButton(window_, button) == GLFW_PRESS;
}

void Window::cursor_position(double& x, double& y) const {
    glfwGetCursorPos(window_, &x, &y);
}

double Window::consume_scroll() {
    const double s = state_.scroll;
    state_.scroll = 0.0;
    return s;
}

double Window::time() const {
    return glfwGetTime();
}

}  // namespace engine::render
