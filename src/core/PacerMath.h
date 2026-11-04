// FlowScale — frame pacing state machine (pure math, no OS dependencies).
//
// Display-side frame generation model:
//   * source frames arrive asynchronously at ~S fps (timestamps recorded on arrival)
//   * the display ticks at D fps; on every tick we choose what to show:
//       - Real      : the newest source frame that arrived since the last Real shown
//       - Generated : an interpolated frame between the two most recent arrivals,
//                     at sub-frame position t in [0, clampMax]
//   * t is computed from arrival timestamps, so each Generated tick lands exactly
//     between the real frames it blends (classic interpolation); with clampMax > 1
//     ("predictive motion") t may run past the newest arrival, extrapolating motion
//     for lower latency at a small ghosting cost.
//
// This header is intentionally dependency-free: it is unit-tested on every OS
// (tests/math_tests.cpp) and shared verbatim by the present thread.
#pragma once
#include <algorithm>

namespace fs {

struct TickDecision {
    enum class Kind { Skip, Real, Generated };
    Kind  kind = Kind::Skip;
    double t = 0.0;   // for Generated: sub-frame position in [0, clampMax]
};

class PacerMath {
public:
    explicit PacerMath(double clampMax = 1.0) : clampMax_(clampMax) {}

    // A new source frame was observed at time `arrival` (monotonic seconds).
    // Returns true if the previous segment closed (useful for stats/tests).
    bool OnSourceFrame(double arrival) {
        const bool hadTwo = n_ == 2;
        if (n_ < 2) {
            a_[n_++] = arrival;
        } else {
            a_[0] = a_[1];
            a_[1] = arrival;
        }
        curShown_ = false;                 // newest arrival must be shown Real first
        return hadTwo;
    }

    // Called once per display tick with the current monotonic time.
    TickDecision Tick(double now) {
        if (n_ == 0) return {};
        if (n_ == 1) {                     // only one frame so far: hold it Real
            if (!curShown_) { curShown_ = true; lastRealTime_ = now; return { TickDecision::Kind::Real, 0.0 }; }
            return {};
        }
        if (!curShown_) {                  // first tick at-or-after the newest arrival
            curShown_ = true;
            lastRealTime_ = now;
            return { TickDecision::Kind::Real, 0.0 };
        }
        const double span = a_[1] - a_[0];
        if (span <= 0.0) return {};        // degenerate timestamps (duplicate arrival)
        double t = (now - a_[0]) / span;
        t = std::clamp(t, 0.0, clampMax_);
        return { TickDecision::Kind::Generated, t };
    }

    bool   HasSegment()  const { return n_ == 2; }
    double LatestArrival() const { return n_ ? a_[n_ - 1] : 0.0; }
    double SegmentSpan() const { return n_ == 2 ? (a_[1] - a_[0]) : 0.0; }  // seconds
    double ClampMax()    const { return clampMax_; }

private:
    double a_[2] = { 0.0, 0.0 };  // arrival times of previous and latest source frame
    int    n_ = 0;
    bool   curShown_ = false;
    double lastRealTime_ = 0.0;
    double clampMax_;
};

} // namespace fs
