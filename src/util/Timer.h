// FlowScale — high-resolution timing helpers (QPC).
#pragma once
#include <windows.h>

namespace fs {

class Timer {
public:
    static void Init() { QueryPerformanceFrequency(&s_freq); }

    // Seconds since an arbitrary epoch; monotonic, double precision, ~100 ns steps.
    static double Now() {
        LARGE_INTEGER c; QueryPerformanceCounter(&c);
        return static_cast<double>(c.QuadPart) / static_cast<double>(s_freq.QuadPart);
    }

    static int64_t Ticks() {
        LARGE_INTEGER c; QueryPerformanceCounter(&c);
        return c.QuadPart;
    }

private:
    inline static LARGE_INTEGER s_freq = [] {
        LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f;
    }();
};

} // namespace fs
