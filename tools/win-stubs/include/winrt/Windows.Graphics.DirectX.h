// winrt/Windows.Graphics.DirectX.h stub — DirectXPixelFormat lives here in the
// real projection (Windows.Graphics.DirectX namespace, NOT ...::Direct3D11).
#pragma once
#include "Windows.Graphics.h"

namespace winrt { namespace Windows { namespace Graphics { namespace DirectX {

enum class DirectXPixelFormat : int32_t {
    R8G8B8A8UIntNormalized = 30,
    B8G8R8A8UIntNormalized = 87,
    R16G16B16A16Float      = 10,
};

}}}} // namespace winrt::Windows::Graphics::DirectX