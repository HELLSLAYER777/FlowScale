#include "capture/WGCCapture.h"
#include "util/Log.h"
#include "util/Timer.h"
#include <winrt/Windows.Foundation.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <dxgi1_2.h>
#include <roapi.h>            // RoGetActivationFactory
#include <winstring.h>        // HStringReference

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

static winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice MakeDirect3DDevice(ID3D11Device* dev) {
    using winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice;
    static IDirect3DDevice cached{ nullptr };
    if (!cached) {
        CPtr<IDXGIDevice> dxgi;
        ThrowIfFailed(dev->QueryInterface(IID_PPV_ARGS(dxgi.GetAddressOf())), "QI dxgidevice");
        // CreateDirect3D11DeviceFromDXGIDevice hands out an IInspectable that IS the
        // IDirect3DDevice interface; wrap it into the projected winrt type.
        winrt::com_ptr<::IInspectable> inspectable;
        ThrowIfFailed(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(), inspectable.put()),
                      "CreateDirect3D11DeviceFromDXGIDevice");
        cached = inspectable.as<IDirect3DDevice>();
    }
    return cached;
}

bool WgcAvailable() {
    try {
        return CaptureInterop() != nullptr;
    } catch (...) { return false; }
}

bool WGCCapture::Start(Device& dev, HWND hwnd) {
    try {
        item_ = nullptr;
        // REFIID overload: winrt::put_abi returns void**, which IID_PPV_ARGS rejects.
        CaptureInterop()->CreateForWindow(hwnd, winrt::guid_of<GraphicsCaptureItem>(),
                                          winrt::put_abi(item_));
        if (!item_) { LOGE("WGC: CreateForWindow failed"); return false; }

        auto size = item_.Size();
        width_ = uint32_t(size.Width); height_ = uint32_t(size.Height);

        // Free-threaded pool: FrameArrived fires on a threadpool thread.
        pool_ = Direct3D11CaptureFramePool::CreateFreeThreaded(MakeDirect3DDevice(dev.Dev()),
                                                               DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                                               2, size);
        // TypedEventHandler<FramePool, IInspectable> — the handler must accept both args.
        arrived_ = pool_.FrameArrived(auto_revoke, [this](auto&& sender, auto&&) {
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

        closedRevoker_ = item_.Closed(auto_revoke, [this](auto&&, auto&&) {
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
    CPtr<ID3D11Texture2D> tex;                        // GetInterface returns an AddRef'd
    tex.Attach(raw);                                  // reference: adopt, don't AddRef again
    return tex;
}

void WGCCapture::Stop() {
    if (arrived_) arrived_.revoke();
    if (closedRevoker_) closedRevoker_.revoke();
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
