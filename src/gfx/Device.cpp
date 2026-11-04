#include "gfx/Device.h"
#include "util/Log.h"
#include <dxgi1_2.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace fs {

bool Device::Init(bool debugLayer) {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (debugLayer) flags |= D3D11_CREATE_DEVICE_DEBUG;

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };
    CPtr<ID3D11Device> dev; CPtr<ID3D11DeviceContext> ctx; D3D_FEATURE_LEVEL got{};

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                   levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                                   dev.GetAddressOf(), &got, ctx.GetAddressOf());
    if (FAILED(hr) && debugLayer) {          // D3D SDK layers absent: retry without
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                               levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                               dev.GetAddressOf(), &got, ctx.GetAddressOf());
    }
    if (FAILED(hr)) { LOGE("D3D11CreateDevice failed 0x%08lX", (unsigned long)hr); return false; }

    dev_ = std::move(dev);
    ctx_ = std::move(ctx);

    CPtr<IDXGIDevice> dxgiDev;
    if (SUCCEEDED(dev_->QueryInterface(IID_PPV_ARGS(dxgiDev.GetAddressOf())))) {
        dxgiDev->GetAdapter(adapter_.ReleaseAndGetAddressOf());
        if (adapter_) {
            CPtr<IDXGIFactory2> f;
            adapter_->GetParent(IID_PPV_ARGS(f.GetAddressOf()));
            factory_ = std::move(f);
        }
    }

    LOGI("D3D11 device ready (FL 0x%04X, adapter %ls)", got, AdapterName().c_str());
    return true;
}

CPtr<ID3D11Texture2D> Device::CreateTexture(uint32_t w, uint32_t h, DXGI_FORMAT fmt,
                                            UINT bind, D3D11_USAGE usage) {
    D3D11_TEXTURE2D_DESC d{};
    d.Width = w; d.Height = h; d.MipLevels = 1; d.ArraySize = 1;
    d.Format = fmt;
    d.SampleDesc.Count = 1;
    d.Usage = usage;
    d.BindFlags = bind;
    if (usage == D3D11_USAGE_STAGING) { d.CPUAccessFlags = D3D11_CPU_ACCESS_READ; d.BindFlags = 0; }

    CPtr<ID3D11Texture2D> tex;
    ThrowIfFailed(dev_->CreateTexture2D(&d, nullptr, tex.GetAddressOf()), "CreateTexture2D");
    return tex;
}

CPtr<ID3D11Buffer> Device::CreateConstantBuffer(size_t bytes) {
    bytes = (bytes + 15) & ~size_t(15);       // 16-byte alignment
    D3D11_BUFFER_DESC d{};
    d.ByteWidth = static_cast<UINT>(bytes);
    d.Usage = D3D11_USAGE_DYNAMIC;
    d.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CPtr<ID3D11Buffer> buf;
    ThrowIfFailed(dev_->CreateBuffer(&d, nullptr, buf.GetAddressOf()), "CreateBuffer(cbuf)");
    return buf;
}

CPtr<ID3D11SamplerState> Device::CreateSampler(bool linear, bool clamp) {
    D3D11_SAMPLER_DESC s{};
    s.Filter = linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
    s.AddressU = s.AddressV = s.AddressW = clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
    s.ComparisonFunc = D3D11_COMPARISON_NEVER;
    s.MaxLOD = D3D11_FLOAT32_MAX;
    CPtr<ID3D11SamplerState> st;
    ThrowIfFailed(dev_->CreateSamplerState(&s, st.GetAddressOf()), "CreateSamplerState");
    return st;
}

std::wstring Device::AdapterName() const {
    if (!adapter_) return L"<unknown>";
    DXGI_ADAPTER_DESC1 d{};
    if (FAILED(adapter_->GetDesc1(&d))) return L"<unknown>";
    return d.Description;
}

} // namespace fs
