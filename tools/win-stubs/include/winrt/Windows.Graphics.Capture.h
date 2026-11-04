// winrt/Windows.Graphics.Capture.h stub
#pragma once
#include "Windows.Graphics.h"
#include "Windows.Graphics.DirectX.Direct3D11.h"
#include <functional>

namespace winrt { namespace Windows { namespace Graphics { namespace DirectX { namespace Direct3D11 {

// IDirect3DSurface projected type (only as<T>() is used on it).
struct IDirect3DSurface {
    IDirect3DSurface() noexcept = default;
    IDirect3DSurface(std::nullptr_t) noexcept {}
    explicit operator bool() const noexcept { return false; }
    // as<ABI interface> yields the raw interface pointer (real cppwinrt behavior)
    template <typename To> To* as() const { return nullptr; }
};

}}}}}

namespace winrt { namespace Windows { namespace Graphics { namespace Capture {

// -- GraphicsCaptureItem --
struct GraphicsCaptureItem {
    GraphicsCaptureItem() noexcept = default;
    GraphicsCaptureItem(std::nullptr_t) noexcept {}
    GraphicsCaptureItem& operator=(std::nullptr_t) noexcept { return *this; }
    explicit operator bool() const noexcept { return false; }
    template <typename To> To as() const { return To{}; }

    Windows::Graphics::SizeInt32 Size() const { return {}; }
    winrt::hstring DisplayName() const { return {}; }

    struct Closed_revoker {
        void revoke() {}
        explicit operator bool() const { return false; }
    };
    template <typename F>
    Closed_revoker Closed(winrt::auto_revoke_t, F&&) { return {}; }
};

// -- Direct3D11CaptureFrame --
struct Direct3D11CaptureFrame {
    Direct3D11CaptureFrame() noexcept = default;
    Direct3D11CaptureFrame(std::nullptr_t) noexcept {}
    Direct3D11CaptureFrame& operator=(std::nullptr_t) noexcept { return *this; }
    explicit operator bool() const noexcept { return false; }

    Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface Surface() const { return {}; }
    Windows::Graphics::SizeInt32 ContentSize() const { return {}; }
};

// -- Direct3D11CaptureFramePool --
struct Direct3D11CaptureFramePool {
    Direct3D11CaptureFramePool() noexcept = default;
    Direct3D11CaptureFramePool(std::nullptr_t) noexcept {}
    Direct3D11CaptureFramePool& operator=(std::nullptr_t) noexcept { return *this; }
    explicit operator bool() const noexcept { return false; }

    static Direct3D11CaptureFramePool CreateFreeThreaded(
        Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        Windows::Graphics::DirectX::DirectXPixelFormat const& format,
        int32_t const& bufferCount,
        Windows::Graphics::SizeInt32 const& size) {
        (void)device; (void)format; (void)bufferCount; (void)size; return {};
    }
    static Direct3D11CaptureFramePool Create(
        Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const&,
        Windows::Graphics::DirectX::DirectXPixelFormat const&,
        int32_t const&, Windows::Graphics::SizeInt32 const&) { return {}; }

    void Recreate(Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
                  Windows::Graphics::DirectX::DirectXPixelFormat const& format,
                  int32_t const& bufferCount,
                  Windows::Graphics::SizeInt32 const& size) const {
        (void)device; (void)format; (void)bufferCount; (void)size;
    }
    void StartCapture() const {}
    void StopCapture() const {}
    Direct3D11CaptureFrame TryGetNextFrame() const { return {}; }

    struct FrameArrived_revoker {
        void revoke() {}
        explicit operator bool() const { return false; }
        // Holding the handler forces the generic-lambda body to be instantiated
        // during the syntax check (same as the real event registration).
        std::function<void(Direct3D11CaptureFramePool const&,
                           Windows::Foundation::IInspectable const&)> handler_;
    };
    template <typename F>
    FrameArrived_revoker FrameArrived(winrt::auto_revoke_t, F&& f) const {
        FrameArrived_revoker r;
        r.handler_ = std::forward<F>(f);
        return r;
    }
};

}}}}

// guid_of needs the complete type.
template <> constexpr winrt::guid winrt::guid_of<
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem>() noexcept {
    return winrt::guid{ { 0x79c32895, 0x247d, 0x4b6a, { 0x9f,0x0c,0x92,0x7c,0x3f,0x6e,0x8c,0x51 } } };
}
