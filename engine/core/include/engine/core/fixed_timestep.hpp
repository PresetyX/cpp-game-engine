#pragma once

#include <algorithm>
#include <cassert>

namespace engine {

// Fixed-timestep accumulator. Pure logic: it never reads a clock, the caller
// supplies the frame time, which keeps it deterministic and testable.
class FixedTimestep {
public:
    // step: simulation step in seconds (> 0).
    // max_frame_time: upper bound on the frame time that is accumulated, so a
    // long stall cannot trigger a runaway catch-up ("spiral of death").
    explicit FixedTimestep(double step, double max_frame_time = 0.25)
        : step_(step), max_frame_time_(max_frame_time) {
        assert(step > 0.0);
        assert(max_frame_time > 0.0);
    }

    // Accumulates frame_dt and returns how many fixed steps to run now.
    int advance(double frame_dt) {
        accumulator_ += std::clamp(frame_dt, 0.0, max_frame_time_);
        int steps = 0;
        while (accumulator_ >= step_) {
            accumulator_ -= step_;
            ++steps;
        }
        return steps;
    }

    // Runs fn(step) once per due step and returns alpha for render interpolation.
    template <typename F>
    double update(double frame_dt, F&& fn) {
        const int steps = advance(frame_dt);
        for (int i = 0; i < steps; ++i) {
            fn(step_);
        }
        return alpha();
    }

    // Fraction of a step left in the accumulator, in [0, 1).
    double alpha() const { return accumulator_ / step_; }

    double step() const { return step_; }
    double max_frame_time() const { return max_frame_time_; }

    void reset() { accumulator_ = 0.0; }

private:
    double step_;
    double max_frame_time_;
    double accumulator_ = 0.0;
};

}  // namespace engine
