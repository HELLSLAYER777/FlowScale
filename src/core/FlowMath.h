// FlowScale — optical-flow reference math (pure C++, no OS/GPU dependencies).
//
// The HLSL compute shaders in src/framegen/shaders implement exactly these
// operations on the GPU; this header is the executable specification used by
// tests/math_tests.cpp to validate constants, kernels and sub-pixel logic.
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace fs::flow {

// ---------------------------------------------------------------- sampling --
// Bilinear sample of a single-channel float image with clamped addressing.
inline float BilinearSample(const std::vector<float>& img, int w, int h, float x, float y) {
    x = std::clamp(x, 0.0f, static_cast<float>(w - 1));
    y = std::clamp(y, 0.0f, static_cast<float>(h - 1));
    const int x0 = static_cast<int>(x), y0 = static_cast<int>(y);
    const int x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
    const float fx = x - static_cast<float>(x0), fy = y - static_cast<float>(y0);
    const float v00 = img[y0 * w + x0], v10 = img[y0 * w + x1];
    const float v01 = img[y1 * w + x0], v11 = img[y1 * w + x1];
    return v00 * (1 - fx) * (1 - fy) + v10 * fx * (1 - fy)
         + v01 * (1 - fx) * fy       + v11 * fx * fy;
}

// SAD between the 8x8 block at (ax,ay) in image a and the block at (bx,by) in b.
inline float Sad8x8(const std::vector<float>& a, const std::vector<float>& b,
                    int w, int h, int ax, int ay, int bx, int by) {
    float sum = 0.0f;
    for (int dy = -4; dy < 4; ++dy) {
        const int ya = std::clamp(ay + dy, 0, h - 1), yb = std::clamp(by + dy, 0, h - 1);
        for (int dx = -4; dx < 4; ++dx) {
            const int xa = std::clamp(ax + dx, 0, w - 1), xb = std::clamp(bx + dx, 0, w - 1);
            sum += std::fabs(a[ya * w + xa] - b[yb * w + xb]);
        }
    }
    return sum / 64.0f;
}

// ------------------------------------------------------------ sub-pixel fit --
// Vertex of the parabola through (-1,s1),(0,s0),(+1,s2): in [-0.5, +0.5] when the
// center sample is the minimum. This is the exact formula used by flow_refine.hlsl.
inline float ParabolaSubPixel(float s1, float s0, float s2) {
    const float denom = s1 - 2.0f * s0 + s2;
    if (denom <= 1e-6f) return 0.0f;
    return std::clamp(0.5f * (s1 - s2) / denom, -0.5f, 0.5f);
}

// --------------------------------------------------------------- lanczos-3 --
// Normalized 2-lobe Lanczos kernel (matches lanczos3.hlsl). Weights over a 6x6
// support sum to 1 by construction of the normalization pass.
inline float Lanczos2(float x) {
    x = std::fabs(x);
    constexpr float PI = 3.14159265358979f;
    if (x < 1e-5f) return 1.0f;
    if (x >= 2.0f) return 0.0f;
    const float px = PI * x;
    return 2.0f * std::sin(px) * std::sin(px / 2.0f) / (px * px);
}

// -------------------------------------------------------------- occlusion ----
// Exponential disagreement weight: 1 when luma values agree, decays to 0 past tau.
// Matches the occlusion weighting in warp_blend.hlsl.
inline float OcclusionWeight(float ya, float yb, float tau, float k) {
    return std::exp(-std::fabs(ya - yb) / std::max(tau, 1e-4f) * k);
}

// ------------------------------------------------- flow field, CPU reference --
// Minimal multi-level block-matching used by tests to validate shader behavior.
// Returns per-grid-cell flow (in *source pixels*, pointing cur -> prev, i.e. the
// offset to add to a current position to find the same content in prev).
struct FlowField {
    int gw = 0, gh = 0;                 // grid size
    int step = 8;                       // source pixels per grid cell
    std::vector<float> u, v;            // flow components
};

inline FlowField EstimateFlow(const std::vector<float>& prevY, const std::vector<float>& curY,
                              int w, int h, int step, int searchRadius) {
    FlowField f;
    f.gw = (w + step - 1) / step;
    f.gh = (h + step - 1) / step;
    f.step = step;
    f.u.assign(static_cast<size_t>(f.gw) * f.gh, 0.0f);
    f.v.assign(static_cast<size_t>(f.gw) * f.gh, 0.0f);

    for (int gy = 0; gy < f.gh; ++gy) {
        for (int gx = 0; gx < f.gw; ++gx) {
            const int cx = gx * step, cy = gy * step;
            float best = 1e30f; float bu = 0.0f, bv = 0.0f;
            for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                    const float sad = Sad8x8(curY, prevY, w, h, cx, cy, cx + dx, cy + dy);
                    if (sad < best) { best = sad; bu = static_cast<float>(dx); bv = static_cast<float>(dy); }
                }
            }
            f.u[gy * f.gw + gx] = bu;
            f.v[gy * f.gw + gx] = bv;
        }
    }
    return f;
}

} // namespace fs::flow
