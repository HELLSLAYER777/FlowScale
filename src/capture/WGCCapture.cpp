#include "capture/WGCCapture.h"
#include "util/Log.h"
#include "util/Timer.h"
#include <winrt/Windows.Foundation.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <dxgi1_2.h>

namespace fs {

using namespace winrt;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;

// ---- WinRT <-> D3D interop helpers (documented COM interfaces, header-only) ----
static IGraphicsCaptureItemInterop* CaptureInterop() {
    static CPtr<IGraphicsCaptureItemInterop> interop = [] {
        CPtr<IGraphicsCaptureItemInterop> p;
        RoGetActivationFactory(HStringReference(L"Windows.Graphics.Capture.GraphicsCaptureItem").Get(),
                               IID_PPV_ARGS(p.GetAddressOf()));
        return p;
    }();
    return interop.Get();
}

static IDirect3DDevice* MakeDirect3DDevice(ID3D11Device* dev) {
    static CPtr<IDirect3DDevice> cached;
    if (!cached) {
        CPtr<IDXGIDevice> dxgi;
        ThrowIfFailed(dev->QueryInterface(IID_PPV_ARGS(dxgi.GetAddressOf())), "QI dxgidevice");
        CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),
            reinterpret_cast<IInspectable**>(winrt::put_abi(cached)));
    }
    return cached.get();
}

bool WgcAvailable() {
    try {
        return CaptureInterop() != nullptr;
    } catch (...) { return false; }
}

bool WGCCapture::Start(Device& dev, HWND hwnd) {
    try {
        item_ = nullptr;
        CaptureInterop()->CreateForWindow(hwnd, IID_PPV_ARGS(winrt::put_abi(item_)));
        if (!item_) { LOGE("WGC: CreateForWindow failed"); return false; }

        auto size = item_.Size();
        width_ = uint32_t(size.Width); height_ = uint32_t(size.Height);

        // Free-threaded pool: FrameArrived fires on a threadpool thread.
        pool_ = Direct3D11CaptureFramePool::CreateFreeThreaded(MakeDirect3DDevice(dev.Dev()),
                                                               DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                                               2, size);
        arrived_ = pool_.FrameArrived(auto_revoke, [this](auto&& sender) {
            auto frame = sender.TryGetNextFrame();
            if (!frame) return;
            {
                std::lock_guard<std::mutex> g(pendingMtx_);
                pending_ = frame;                    // hold ref; copied on present thread
                pendingTime_ = Timer::Now();
            }
            // Drain older frames so latency stays at exactly one capture frame.
            while (auto older = sender.TryGetNextFrame()) {
                std::lock_guard<std::mutex> g(pendingMtx_);
                pending_ = older;
                pendingTime_ = Timer::Now();
            }
        });

        closed_ = item_.Closed(auto_revoke, [this](auto&&, auto&&) {
            closed_.store(true, std::memory_order_relaxed);
        });

        // Keep the pool resized to the window while content size changes.
        auto contentSize = item_.Size();
        if (contentSize.Width != int(width_) || contentSize.Height != int(height_))
            RecreateFramePool(dev, contentSize);

        pool_.StartCapture();
        LOGI("WGC capturing %ls (%ux%u)", item_.DisplayName().c_str(), width_, height_);
        return true;
    } catch (const hresult_error& e) {
        LOGE("WGC start failed: %ls", e.message().c_str());
        return false;
    }
}

void WGCCapture::RecreateFramePool(Device& dev, const Windows::Graphics::SizeInt32& size) {
    pool_.Recreate(MakeDirect3DDevice(dev.Dev()), DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
    width_ = uint32_t(size.Width); height_ = uint32_t(size.Height);
    LOGI("WGC frame pool recreated at %ux%u", width_, height_);
}

CPtr<ID3D11Texture2D> WGCCapture::TakePendingSurface(double* arrivalTimeOut) {
    winrt::agile_ref<Direct3D11CaptureFrame> frame;
    {
        std::lock_guard<std::mutex> g(pendingMtx_);
        frame = pending_;
        *arrivalTimeOut = pendingTime_;
        pending_ = nullptr;                          // consumed
    }
    if (!frame) return nullptr;
    auto f = frame.get();
    if (!f) return nullptr;
    auto access = f.Surface().as<IDirect3DDxgiInterfaceAccess>();
    ID3D11Texture2D* raw = nullptr;
    ThrowIfFailed(access->GetInterface(IID_PPV_ARGS(&raw)), "GetInterface(capture surface)");
    return CPtr<ID3D11Texture2D>(raw);               // ComPtr adopts the reference
}

void WGCCapture::Stop() {
    if (arrived_) arrived_.revoke();
    if (closed_)  closed_.revoke();
    if (pool_) { try { pool_.StopCapture(); } catch (...) {} pool_ = nullptr; }
    item_ = nullptr;
    std::lock_guard<std::mutex> g(pendingMtx_);
    pending_ = nullptr;
}

std::unique_ptr<ICapture> CreateCapture(Device& dev, HWND hwnd) {
    UNREFERENCED_PARAMETER(dev);
    wchar_t cls[64];
    GetClassNameW(hwnd, cls, 64);
    const bool isDesktop = _wcsicmp(cls, L"Progman") == 0 || _wcsicmp(cls, L"WorkerW") == 0;
    if (!isDesktop && WgcAvailable()) return std::make_unique<WGCCapture>();
    return nullptr;    // Engine falls back to DDA for desktop/monitor targets
}

} // namespace fs
