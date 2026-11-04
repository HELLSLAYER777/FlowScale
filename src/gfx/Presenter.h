// FlowScale — borderless fullscreen presenter with waitable-swapchain pacing.
//
// The presenter owns a WS_POPUP topmost window covering the monitor that contains
// the target window, and a flip-model swapchain with max frame latency 1. The
// present thread calls WaitTick() every display period — that wait is the pacing
// source for the whole engine.
#pragma once
#include "gfx/Device.h"
#include "gfx/Hud.h"
#include "core/Stats.h"
#include "util/Common.h"

namespace fs {

class Presenter {
public:
    ~Presenter();

    // Creates the window + swapchain on `monitor` at desktop resolution.
    bool Init(Device& dev, HMONITOR monitor, bool tryHdr);
    void Shutdown();

    // Blocks until the swapchain allows the next present; returns tick time (Timer::Now()).
    double WaitTick();
    // Optional additional frame cap (seconds between frames); returns actual wait time.
    double WaitTick(double minInterval);

    // Copies `frame` to the backbuffer, draws the HUD if wired, and presents.
    void SetHud(Hud* hud) { hud_ = hud; }
    void Present(ID3D11Texture2D* frame, bool vsync, const HudState* hudState = nullptr);

    HWND      Hwnd()     const { return hwnd_; }
    uint32_t  Width()    const { return width_; }
    uint32_t  Height()   const { return height_; }
    bool      HdrActive()const { return hdr_; }

private:
    bool CreateSwapchain(bool tryHdr);
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    Device*  dev_ = nullptr;
    HWND     hwnd_ = nullptr;
    uint32_t width_ = 0, height_ = 0;
    bool     hdr_ = false;
    Hud*     hud_ = nullptr;

    CPtr<IDXGISwapChain1>        swap_;
    CPtr<IDXGISwapChain2>        swap2_;      // waitable + frame latency control
    HANDLE                       waitable_ = nullptr;
    CPtr<ID3D11Texture2D>        backbuf_;
    bool                         vsyncOn_ = true;
};

} // namespace fs
