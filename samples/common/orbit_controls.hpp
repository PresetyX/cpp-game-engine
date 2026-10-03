#pragma once

// Mouse orbit/zoom shared by the samples: left button drag orbits, the wheel zooms.
// Sample-only code; it is not part of the engine (which has no input module yet).

#include <GLFW/glfw3.h>

#include "engine/render/camera.hpp"
#include "engine/render/window.hpp"

namespace sample {

class OrbitControls {
public:
    explicit OrbitControls(engine::render::Window& window) { window.cursor_position(x_, y_); }

    void update(engine::render::Window& window, engine::render::OrbitCamera& camera) {
        double x = 0.0;
        double y = 0.0;
        window.cursor_position(x, y);
        if (window.mouse_button_down(GLFW_MOUSE_BUTTON_LEFT)) {
            camera.orbit(static_cast<float>(-(x - x_)) * 0.005f,
                         static_cast<float>(y - y_) * 0.005f);
        }
        x_ = x;
        y_ = y;
        const double scroll = window.consume_scroll();
        if (scroll != 0.0) {
            camera.zoom(scroll > 0.0 ? 0.9f : 1.1f);
        }
    }

private:
    double x_ = 0.0;
    double y_ = 0.0;
};

// True once per key press (edge), for keys like Space and R.
class KeyPress {
public:
    explicit KeyPress(int key) : key_(key) {}
    bool pressed(const engine::render::Window& window) {
        const bool down = window.key_down(key_);
        const bool edge = down && !was_down_;
        was_down_ = down;
        return edge;
    }

private:
    int key_;
    bool was_down_ = false;
};

}  // namespace sample
