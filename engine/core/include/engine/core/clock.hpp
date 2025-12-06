#pragma once

namespace engine {

struct Clock {
    // Monotonic time in seconds since an arbitrary epoch.
    static double now_seconds();
};

// Measures the time between successive tick() calls.
class FrameTimer {
public:
    // Returns seconds since the previous tick(); 0 on the first call.
    double tick();

private:
    double last_ = 0.0;
    bool started_ = false;
};

}  // namespace engine
