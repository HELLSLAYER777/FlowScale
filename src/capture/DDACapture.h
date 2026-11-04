// FlowScale — DXGI Desktop Duplication backend (full-monitor capture).
// Used for desktop targets and Windows builds without Windows.Graphics.Capture.
#pragma once
#include "capture/ICapture.h"
#include <dxgi1_2.h>

namespace fs {

class DDACapture final : public ICapture {
public:
    ~DDACapture() override { Stop(); }

    bool Start(Device& dev, HWND hwnd) override;   // hwnd = any window; its monitor is used
    void Stop() override;
    bool IsMonitor() const override { return true; }
    bool PollFrame(ID3D11Texture2D* out) override;
    uint32_t Width()  const override { return width_; }
    uint32_t Height() const override { return height_; }

private:
    CPtr<IDXGIOutputDuplication> dup_;
    CPtr<ID3D11Texture2D>        staged_;
    CPtr<ID3D11DeviceContext>    ctx_;    // borrowed from Device; present-thread use only
    uint32_t width_ = 0, height_ = 0;
    bool haveFrame_ = false;
};

} // namespace fs
