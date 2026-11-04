// FlowScale — Windows.Graphics.Capture backend (per-window capture, Win10 19041+).
//
// Frame lifecycle: the OS threadpool delivers surfaces created on OUR D3D11 device.
// onFrame only stores (surface, timestamp) — the present thread does all D3D work,
// keeping the immediate context single-threaded by design.
#pragma once
#include "capture/ICapture.h"
#include <atomic>
#include <mutex>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

namespace fs {

class WGCCapture final : public ICapture {
public:
    ~WGCCapture() override { Stop(); }

    bool Start(Device& dev, HWND hwnd) override;
    void Stop() override;
    bool IsMonitor() const override { return false; }
    uint32_t Width()  const override { return width_; }
    uint32_t Height() const override { return height_; }

    // Present-thread API: pops the newest frame surface (caller copies, then drops
    // the returned ComPtr). Returns null when nothing arrived.
    CPtr<ID3D11Texture2D> TakePendingSurface(double* arrivalTimeOut);
    // True when the user closed the captured window; engine stops itself.
    bool TargetClosed() const { return closed_.load(std::memory_order_relaxed); }

private:
    void RecreateFramePool(Device& dev, const winrt::Windows::Graphics::SizeInt32& size);

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem        item_{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool pool_{ nullptr };
    // { nullptr } required: default-init of a projected class is ambiguous
    // in this SDK, which would delete WGCCapture's implicit default ctor.
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession     session_{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker arrived_;
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem::Closed_revoker closedRevoker_;

    // Refcounted frame copy; safe to hand across threads because the process is
    // initialized as MTA (see main.cpp), so WGC objects are free-threaded.
    // { nullptr } required here too (same reason as item_/pool_/session_):
    // without it the implicit WGCCapture default ctor is deleted and every
    // std::make_unique<WGCCapture>() fails to compile (MSVC C2280).
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame pending_{ nullptr };
    std::mutex pendingMtx_;
    double   pendingTime_ = 0.0;
    uint32_t width_ = 0, height_ = 0;
    std::atomic<bool> closed_{ false };
};

} // namespace fs
