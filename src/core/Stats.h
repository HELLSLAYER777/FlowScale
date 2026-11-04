// FlowScale — live statistics + HUD presentation state.
#pragma once
#include <atomic>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace fs {

// Fixed-capacity ring of samples in seconds, with percentile queries.
class MetricRing {
public:
    explicit MetricRing(size_t capacity = 240) : buf_(capacity) {}

    void Push(double v) {
        buf_[head_] = v;
        head_ = (head_ + 1) % buf_.size();
        if (count_ < buf_.size()) ++count_;
    }
    void Clear() { head_ = 0; count_ = 0; }
    size_t Count() const { return count_; }

    // Percentile in 0..1 over the current window (approximate: partial sort copy).
    double Percentile(double p) const;
    double Mean() const;

private:
    std::vector<double> buf_;
    size_t head_ = 0, count_ = 0;
};

// Snapshot the present thread pushes to the UI/HUD once per interval.
struct HudState {
    float srcFps = 0;       // measured source (capture) FPS
    float outFps = 0;       // presented FPS
    float genFps = 0;       // generated-frame share of outFps
    float latencyP50 = 0;   // capture->present ms
    float latencyP95 = 0;
    bool  neuralActive = false;
    std::wstring fgLabel = L"Off";
    std::wstring algoLabel;
    std::wstring status = L"Stopped";
};

class Stats {
public:
    // Present-thread side --------------------------------------------------
    void OnSourceFrame(double arrivalTime)      { srcArrivals_.Push(arrivalTime); }
    void OnPresent(double submitTime)           { presents_.Push(submitTime); }
    void OnGenerated()                          { ++genCount_; }
    void OnFrameLatency(double seconds)         { latency_.Push(seconds); }

    // Exponential moving averages for FPS reporting.
    void SetSrcFps(float fps)  { srcFps_.store(fps, std::memory_order_relaxed); }
    void SetOutFps(float fps)  { outFps_.store(fps, std::memory_order_relaxed); }

    // Reader side (UI/HUD) --------------------------------------------------
    HudState Snapshot() const;
    MetricRing& Latency() { return latency_; }

private:
    MetricRing srcArrivals_{ 240 }, presents_{ 240 }, latency_{ 240 };
    std::atomic<float> srcFps_{ 0 }, outFps_{ 0 };
    mutable std::atomic<uint32_t> genCount_{ 0 };
};

} // namespace fs
