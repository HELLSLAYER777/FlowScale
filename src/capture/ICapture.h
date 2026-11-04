// FlowScale — capture backend interface.
#pragma once
#include <memory>
#include <functional>
#include "gfx/Device.h"
#include "util/Common.h"

namespace fs {

class ICapture {
public:
    virtual ~ICapture() = default;

    // Starts capturing `hwnd` (a window for WGC, or its monitor for DDA).
    virtual bool Start(Device& dev, HWND hwnd) = 0;
    virtual void Stop() = 0;

    // True when this backend captures a whole monitor (DDA) rather than a window (WGC).
    virtual bool IsMonitor() const = 0;

    // DDA backends poll: copy the latest desktop frame into `out` (engine-owned texture)
    // and return true when a new frame was acquired this tick. WGC ignores this.
    virtual bool PollFrame(ID3D11Texture2D* out) { UNREFERENCED_PARAMETER(out); return false; }

    // WGC backends push: called on a threadpool thread; implementations must not
    // make D3D11 calls here — only store the surface + timestamp.
    std::function<void(ID3D11Texture2D* surface, double arrivalTime)> onFrame;

    // Reported source size (may change on window resize; engine re-inits when needed).
    virtual uint32_t Width()  const = 0;
    virtual uint32_t Height() const = 0;
};

// Picks the best backend for hwnd: WGC when available and window != desktop;
// DDA for monitor targets or systems without WGC.
bool WgcAvailable();
std::unique_ptr<ICapture> CreateCapture(Device& dev, HWND hwnd);

} // namespace fs
