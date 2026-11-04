// FlowScale pure-math test suite.
//
// Deliberately dependency-free (no Windows/D3D headers): it validates the
// pacing state machine and the optical-flow reference math that the HLSL
// shaders implement, and it builds on any OS (CI runs it on Linux).
//
//   cmake -B build-tests -DFLOWSCALE_BUILD_TESTS=ON && cmake --build build-tests && ctest
#include "../src/core/PacerMath.h"
#include "../src/core/FlowMath.h"

#include <cmath>
#include <cstdio>
#include <random>

using namespace fs;

static int g_failed = 0;
#define CHECK(cond) do { \
    if (!(cond)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++g_failed; } \
} while (0)

static bool Near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) <= eps; }

// ------------------------------------------------------------------ pacing --
static void TestPacerRealFramesShownOncePerArrival() {
    PacerMath p;
    p.OnSourceFrame(0.0);
    auto d0 = p.Tick(0.016);                    // first tick: show the only frame
    CHECK(d0.kind == TickDecision::Kind::Real);

    p.OnSourceFrame(0.033);
    auto d1 = p.Tick(0.033);                    // arrival tick: Real
    CHECK(d1.kind == TickDecision::Kind::Real);
    auto d2 = p.Tick(0.041);                    // next tick: Generated t in (0,1]
    CHECK(d2.kind == TickDecision::Kind::Generated);
    CHECK(d2.t > 0.0 && d2.t <= 1.0);
    auto d3 = p.Tick(0.049);                    // non-strict monotonic (clamps at 1 = hold)
    CHECK(d3.kind == TickDecision::Kind::Generated);
    CHECK(d3.t >= d2.t);
}

static void TestPacerMonotonicAndBounded() {
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> jitter(0.0, 0.004);
    PacerMath p(1.0);
    double now = 0.0, lastT = -1.0;
    double arrival = 0.0;
    p.OnSourceFrame(arrival);
    for (int i = 0; i < 20000; ++i) {
        now += 1.0 / 240.0;                     // 240 Hz display ticks
        if (now >= arrival + 1.0 / 60.0 + jitter(rng)) {
            arrival += 1.0 / 60.0;              // 60 Hz source
            p.OnSourceFrame(arrival);
        }
        const auto d = p.Tick(now);
        if (d.kind == TickDecision::Kind::Generated) {
            CHECK(d.t >= 0.0 && d.t <= 1.0);
            CHECK(d.t >= lastT - 1e-9);         // monotonic within segment
            lastT = d.t;
        } else if (d.kind == TickDecision::Kind::Real) {
            lastT = -1.0;                       // reset per segment
        }
    }
}

static void TestPacerPredictiveExtrapolation() {
    PacerMath p(1.5);
    p.OnSourceFrame(0.0);
    p.Tick(0.0);
    p.OnSourceFrame(1.0);
    p.Tick(1.0);
    const auto d = p.Tick(1.6);                 // t = 1.6 -> clamped to 1.5
    CHECK(d.kind == TickDecision::Kind::Generated);
    CHECK(Near(d.t, 1.5, 1e-9));
}

static void TestPacerDuplicateArrivals() {
    PacerMath p;
    p.OnSourceFrame(1.0);
    p.OnSourceFrame(1.0);                       // degenerate: zero span
    p.Tick(1.0);
    const auto d = p.Tick(1.1);
    CHECK(d.kind == TickDecision::Kind::Generated || d.kind == TickDecision::Kind::Skip);
}

// --------------------------------------------------------------- flow math --
static void TestParabolaSubPixel() {
    CHECK(Near(fs::flow::ParabolaSubPixel(2.0f, 1.0f, 2.0f), 0.0f));
    // Parabola through (-1,3),(0,1),(1,2): vertex at x = 1/6 (slightly right).
    CHECK(Near(fs::flow::ParabolaSubPixel(3.0f, 1.0f, 2.0f), 1.0f / 6.0f, 1e-6f));
    // Symmetric is 0; flat is 0.
    CHECK(Near(fs::flow::ParabolaSubPixel(1.0f, 1.0f, 1.0f), 0.0f));
    // Clamped to +/-0.5.
    CHECK(std::fabs(fs::flow::ParabolaSubPixel(4.0f, 0.0f, 2.0f)) <= 0.5f + 1e-6f);
}

static void TestBilinearSample() {
    std::vector<float> img = {
        0, 10,
        20, 30,
    };
    CHECK(Near(fs::flow::BilinearSample(img, 2, 2, 0, 0), 0.0f));
    CHECK(Near(fs::flow::BilinearSample(img, 2, 2, 1, 0), 10.0f));
    CHECK(Near(fs::flow::BilinearSample(img, 2, 2, 0.5, 0), 5.0f));
    CHECK(Near(fs::flow::BilinearSample(img, 2, 2, 0.5, 0.5), 15.0f));
    // Clamped addressing outside the image.
    CHECK(Near(fs::flow::BilinearSample(img, 2, 2, -3, 0), 0.0f));
    CHECK(Near(fs::flow::BilinearSample(img, 2, 2, 5, 5), 30.0f));
}

static void TestSad() {
    std::vector<float> a(16 * 16, 0.5f), b(16 * 16, 0.5f);
    CHECK(Near(fs::flow::Sad8x8(a, b, 16, 16, 8, 8, 8, 8), 0.0f));
    for (auto& v : b) v = 0.75f;
    CHECK(Near(fs::flow::Sad8x8(a, b, 16, 16, 8, 8, 8, 8), 0.25f));
}

static void TestFlowEstimateRecover() {
    // Translate a random-noise pattern by (2, 1) px; the estimator must recover it.
    // (Noise has a unique SAD minimum, unlike e.g. periodic gradients which are
    // ambiguous by construction — a real lesson from running this test.)
    const int W = 64, H = 64;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> val(0.0f, 16.0f);
    std::vector<float> prev(size_t(W * H)), cur(size_t(W * H));
    for (auto& v : prev) v = val(rng);

    const int dx = 2, dy = 1;                    // content moved by (dx, dy)
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            const int sx = std::clamp(x - dx, 0, W - 1), sy = std::clamp(y - dy, 0, H - 1);
            cur[size_t(y) * W + x] = prev[size_t(sy) * W + sx];
        }

    const auto f = fs::flow::EstimateFlow(prev, cur, W, H, /*step*/8, /*radius*/4);
    // Check the cells far from borders.
    int ok = 0, total = 0;
    for (int gy = 1; gy < f.gh - 1; ++gy)
        for (int gx = 1; gx < f.gw - 1; ++gx) {
            ++total;
            if (Near(f.u[size_t(gy) * f.gw + gx], float(-dx), 1e-3f) &&
                Near(f.v[size_t(gy) * f.gw + gx], float(-dy), 1e-3f))
                ++ok;
        }
    CHECK(total > 0);
    // Flow points cur -> prev (hence negative); interior cells must all agree.
    CHECK(ok == total);
}

static void TestLanczosNormalization() {
    // Discrete kernel over the 6-tap support must sum ~1 at integer offsets.
    float sum = 0;
    for (int i = -2; i <= 3; ++i) sum += fs::flow::Lanczos2(float(i));
    CHECK(std::fabs(sum - 1.0f) < 0.02f);
}

static void TestOcclusionWeight() {
    CHECK(Near(fs::flow::OcclusionWeight(0.5f, 0.5f, 0.1f, 10.0f), 1.0f));
    CHECK(fs::flow::OcclusionWeight(0.0f, 1.0f, 0.1f, 10.0f) < 1e-3f);
}

int main() {
    TestPacerRealFramesShownOncePerArrival();
    TestPacerMonotonicAndBounded();
    TestPacerPredictiveExtrapolation();
    TestPacerDuplicateArrivals();
    TestParabolaSubPixel();
    TestBilinearSample();
    TestSad();
    TestFlowEstimateRecover();
    TestLanczosNormalization();
    TestOcclusionWeight();

    if (g_failed == 0) { printf("all math tests passed\n"); return 0; }
    printf("%d test(s) FAILED\n", g_failed);
    return 1;
}
