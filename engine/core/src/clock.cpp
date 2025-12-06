#include "engine/core/clock.hpp"

#include <chrono>

namespace engine {

double Clock::now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

double FrameTimer::tick() {
    const double now = Clock::now_seconds();
    const double delta = started_ ? now - last_ : 0.0;
    last_ = now;
    started_ = true;
    return delta;
}

}  // namespace engine
