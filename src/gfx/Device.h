// FlowScale — D3D11 device wrapper (single device shared by capture/present/passes).
#pragma once
#include "util/Common.h"

namespace fs {

class Device {
public:
    bool Init(bool debugLayer);
    bool IsValid() const { return dev_ != nullptr; }

    ID3D11Device*        Dev()  const { return dev_.Get(); }
    ID3D11DeviceContext* Ctx()  const { return ctx_.Get(); }
    IDXGIFactory2*       Factory() const { return factory_.Get(); }
    IDXGIAdapter1*       Adapter() const { return adapter_.Get(); }

    CPtr<ID3D11Texture2D> CreateTexture(uint32_t w, uint32_t h, DXGI_FORMAT fmt,
                                        UINT bind, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
    CPtr<ID3D11Buffer>    CreateConstantBuffer(size_t bytes);
    CPtr<ID3D11SamplerState> CreateSampler(bool linear, bool clamp = true);

    // Adapter description for the UI / HUD.
    std::wstring AdapterName() const;

private:
    CPtr<ID3D11Device>        dev_;
    CPtr<ID3D11DeviceContext> ctx_;
    CPtr<IDXGIAdapter1>       adapter_;
    CPtr<IDXGIFactory2>       factory_;
};

} // namespace fs
