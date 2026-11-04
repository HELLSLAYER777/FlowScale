#include "core/Stats.h"
#include <algorithm>
#include <numeric>

namespace fs {

double MetricRing::Percentile(double p) const {
    if (count_ == 0) return 0.0;
    std::vector<double> sorted;
    sorted.reserve(count_);
    const size_t start = (head_ + buf_.size() - count_) % buf_.size();
    for (size_t i = 0; i < count_; ++i) sorted.push_back(buf_[(start + i) % buf_.size()]);
    std::sort(sorted.begin(), sorted.end());
    const size_t idx = std::min(sorted.size() - 1,
                                static_cast<size_t>(std::clamp(p, 0.0, 1.0) * (sorted.size() - 1)));
    return sorted[idx];
}

double MetricRing::Mean() const {
    if (count_ == 0) return 0.0;
    double sum = 0;
    const size_t start = (head_ + buf_.size() - count_) % buf_.size();
    for (size_t i = 0; i < count_; ++i) sum += buf_[(start + i) % buf_.size()];
    return sum / static_cast<double>(count_);
}

HudState Stats::Snapshot() const {
    HudState h;
    h.srcFps     = srcFps_.load(std::memory_order_relaxed);
    h.outFps     = outFps_.load(std::memory_order_relaxed);
    h.latencyP50 = static_cast<float>(latency_.Percentile(0.50) * 1000.0);
    h.latencyP95 = static_cast<float>(latency_.Percentile(0.95) * 1000.0);
    const uint32_t g = genCount_.exchange(0, std::memory_order_relaxed);
    const float total = presents_.Mean() > 0 ? static_cast<float>(1.0 / presents_.Mean()) : 0.f;
    h.genFps = total > 0 ? std::min(total, static_cast<float>(g)) : 0.f;
    return h;
}

} // namespace fs
