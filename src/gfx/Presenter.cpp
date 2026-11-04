#include "gfx/Presenter.h"
#include "util/Log.h"
#include "util/Timer.h"
#include <dxgi1_5.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace fs {

static const wchar_t* kPresenterClass = L"FlowScalePresenterWnd";

Presenter::~Presenter() { Shutdown(); }

LRESULT CALLBACK Presenter::WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_NCHITTEST:          return HTCLIENT;      // never draggable mid-run
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) ShowWindow(h, SW_MINIMIZE);
        return 0;
    case WM_DESTROY:            return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

bool Presenter::Init(Device& dev, HMONITOR monitor, bool tryHdr) {
    dev_ = &dev;

    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc{ sizeof(wc) };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = kPresenterClass;
        if (!RegisterClassExW(&wc)) { LOGE("RegisterClassEx failed"); return false; }
        classRegistered = true;
    }

    MONITORINFO mi{ sizeof(mi) };
    if (!GetMonitorInfoW(monitor, &mi)) { LOGE("GetMonitorInfo failed"); return false; }
    const LONG mx = mi.rcMonitor.left,  my = mi.rcMonitor.top;
    width_  = static_cast<uint32_t>(mi.rcMonitor.right - mx);
    height_ = static_cast<uint32_t>(mi.rcMonitor.bottom - my);

    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                            kPresenterClass, L"FlowScale", WS_POPUP,
                            mx, my, static_cast<int>(width_), static_cast<int>(height_),
                            nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!hwnd_) { LOGE("Create presenter window failed"); return false; }

    if (!CreateSwapchain(tryHdr)) { Shutdown(); return false; }

    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    LOGI("Presenter %ux%u on monitor %p (HDR=%d)", width_, height_, (void*)monitor, hdr_);
    return true;
}

bool Presenter::CreateSwapchain(bool tryHdr) {
    DXGI_SWAP_CHAIN_DESC1 d{};
    d.Width = width_; d.Height = height_;
    d.Format = tryHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;
    d.SampleDesc.Count = 1;
    d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d.BufferCount = 2;                                  // minimum latency
    d.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    d.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    CPtr<IDXGIFactory2> f = dev_->Factory();
    HRESULT hr = f->CreateSwapChainForHwnd(dev_->Dev(), hwnd_, &d, nullptr, nullptr, swap_.GetAddressOf());
    if (FAILED(hr)) { LOGE("CreateSwapChainForHwnd failed 0x%08lX", (unsigned long)hr); return false; }

    ThrowIfFailed(f->MakeWindowAssociation(hwnd_, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");

    hdr_ = false;
    if (tryHdr) {
        // Accept the float16 swapchain only when the display advertises advanced color.
        CPtr<IDXGISwapChain3> s3;
        if (SUCCEEDED(swap_.QueryInterface(IID_PPV_ARGS(s3.GetAddressOf())))) {
            DXGI_COLOR_SPACE_TYPE support[32]{};
            UINT n = 0;
            if (SUCCEEDED(s3->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &n))
                && (n & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
                ThrowIfFailed(s3->SetColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020), "SetColorSpace");
                hdr_ = true;
            }
        }
        if (!hdr_) LOGW("HDR requested but display/driver declined; running SDR float16 path");
    }

    if (SUCCEEDED(swap_.QueryInterface(IID_PPV_ARGS(swap2_.GetAddressOf())))) {
        ThrowIfFailed(swap2_->SetMaximumFrameLatency(1), "SetMaximumFrameLatency");
        waitable_ = swap2_->GetFrameLatencyWaitableObject();
    } else {
        LOGW("Waitable swapchain unavailable; pacing falls back to vsync only");
    }

    ThrowIfFailed(swap_->GetBuffer(0, IID_PPV_ARGS(backbuf_.ReleaseAndGetAddressOf())), "GetBuffer");
    return true;
}

double Presenter::WaitTick() { return WaitTick(0.0); }

double Presenter::WaitTick(double minInterval) {
    const double t0 = Timer::Now();
    if (waitable_) {
        // 1-frame-latency waitable: blocks until the previous present retires a slot.
        WaitForSingleObject(waitable_, 100);
    }
    if (minInterval > 0.0) {
        const double target = t0 + minInterval;
        double spin = Timer::Now();
        while (spin < target) { Sleep(1); spin = Timer::Now(); }
    }
    return Timer::Now();
}

void Presenter::Present(ID3D11Texture2D* frame, bool vsync, const HudState* hudState) {
    if (!swap_ || !frame) return;
    ID3D11DeviceContext* ctx = dev_->Ctx();
    ctx->CopyResource(backbuf_.Get(), frame);
    if (hud_ && hudState) hud_->Draw(backbuf_.Get(), *hudState);
    ThrowIfFailed(swap_->Present(vsync ? 1 : 0, 0), "Present");
}

void Presenter::Shutdown() {
    backbuf_.Reset();
    swap2_.Reset();
    swap_.Reset();
    if (waitable_) { CloseHandle(waitable_); waitable_ = nullptr; }
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    dev_ = nullptr;
}

} // namespace fs
