// FlowScale — scaling pipeline: [CAS pre-sharpen] -> [scale pass] -> [RCAS post].
//
// The scale pass is a runtime-compiled compute shader following the standard
// contract (see docs/SHADERS.md): t0 source, u0 destination, b0 ShaderParamsCB.
// Builtin passes live in <exe>\shaders\scaling\; user passes in <exe>\shaders\user\
// and hot-reload with F5.
#pragma once
#include "gfx/CShader.h"
#include "core/Settings.h"

namespace fs {

class ScalerChain {
public:
    bool Init(Device& dev, const Settings& s, uint32_t srcW, uint32_t srcH,
              uint32_t dstW, uint32_t dstH, DXGI_FORMAT srcFmt, DXGI_FORMAT dstFmt);
    void Shutdown();

    // Rebuilds passes when algorithm / sizes / user shader changed. Returns false on error.
    bool ApplySettings(const Device& dev, const Settings& s,
                       uint32_t srcW, uint32_t srcH, uint32_t dstW, uint32_t dstH);

    // F5: recompile changed user shaders in place, keep last good pipeline on error.
    void HotReloadUserShader(ID3D11Device* dev);

    // Processes src -> dst on the caller's immediate context.
    void Process(ID3D11Texture2D* src, ID3D11Texture2D* dst);

    const std::wstring& StatusText() const { return status_; }

private:
    bool Build(Device& dev, const Settings& s);
    const Device* dev_ = nullptr;

    // GPU resources
    CPtr<ID3D11Texture2D>       tmp_;          // CAS output / RCAS input
    CPtr<ID3D11ShaderResourceView> tmpSrv_;
    CPtr<ID3D11UnorderedAccessView> tmpUav_;
    CPtr<ID3D11ShaderResourceView> srcSrv_;
    CPtr<ID3D11UnorderedAccessView> dstUav_;
    CPtr<ID3D11Buffer>          cb_;
    CPtr<ID3D11SamplerState>    samPoint_, samLinear_;

    CShader cas_, rcas_, scale_;
    CPtr<ID3D11ComputeShader> casCs_, rcasCs_;
    DXGI_FORMAT fmt_ = DXGI_FORMAT_B8G8R8A8_UNORM;   // intermediate/destination format

    // Config
    ScaleAlgo algo_ = ScaleAlgo::Lanczos3;
    std::wstring userShader_;
    uint32_t sw_ = 0, sh_ = 0, dw_ = 0, dh_ = 0;
    float casAmount_ = 0, rcasAmount_ = 0;
    bool  useCas_ = false, useRcas_ = false;

    std::wstring status_ = L"ok";
};

} // namespace fs
