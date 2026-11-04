// winrt/Windows.Graphics.DirectX.Direct3D11.h stub
#pragma once
#include "Windows.Graphics.h"

// ABI interface declared by the SDK's windows.graphics.directx.direct3d11.h
struct IDirect3DDevice;   // global-namespace ABI type (IInspectable-derived)

namespace winrt { namespace Windows { namespace Graphics { namespace DirectX {

enum class DirectXPixelFormat : int32_t {
    R8G8B8A8UIntNormalized = 30,
    B8G8R8A8UIntNormalized = 87,
    R16G16B16A16Float = 10,
};

namespace Direct3D11 {

// Projected IDirect3DDevice (consumes ::IDirect3DDevice).
struct IDirect3DDevice {
    using abi_type = ::IDirect3DDevice;
    IDirect3DDevice() noexcept = default;
    IDirect3DDevice(std::nullptr_t) noexcept {}
    IDirect3DDevice& operator=(std::nullptr_t) noexcept { return *this; }
    explicit operator bool() const noexcept { return false; }
    template <typename To> To as() const { return To{}; }
};

}}}}} // namespace winrt::Windows::Graphics::DirectX::Direct3D11

FS_UUID(winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice,
        0x45d9b8fa, 0x5b00, 0x4b0f, 0xb4,0x68,0x0d,0x2a,0x48,0xc5,0x8d,0xd1)
