#include "scaling/ScalerChain.h"
#include "util/Common.h"
#include "util/Log.h"
#include <algorithm>

namespace fs {

static std::wstring ShaderRoot() { return ShaderDir() + L"\\scaling"; }

bool ScalerChain::Init(Device& dev, const Settings& s, uint32_t srcW, uint32_t srcH,
                       uint32_t dstW, uint32_t dstH, DXGI_FORMAT srcFmt, DXGI_FORMAT dstFmt) {
    dev_ = &dev;
    cb_        = dev.CreateConstantBuffer(sizeof(ShaderParamsCB));
    samPoint_  = dev.CreateSampler(false);
    samLinear_ = dev.CreateSampler(true);

    sw_ = srcW; sh_ = srcH; dw_ = dstW; dh_ = dstH;
    fmt_ = dstFmt;

    D3D11_TEXTURE2D_DESC td{};
    td.Width = dstW; td.Height = dstH; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = dstFmt; td.SampleDesc.Count = 1;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    if (FAILED(dev.Dev()->CreateTexture2D(&td, nullptr, tmp_.GetAddressOf()))) {
        status_ = L"failed to create intermediate texture";
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = dstFmt; sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
    ud.Format = dstFmt; ud.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    dev.Dev()->CreateShaderResourceView(tmp_.Get(), &sd, tmpSrv_.GetAddressOf());
    dev.Dev()->CreateUnorderedAccessView(tmp_.Get(), &ud, tmpUav_.GetAddressOf());

    // Shared sharpening passes (compiled once per session).
    const std::wstring root = ShaderRoot();
    if (!cas_.Compile(dev.Dev(), root + L"\\cas.hlsl"))  { status_ = L"CAS compile failed"; return false; }
    if (!rcas_.Compile(dev.Dev(), root + L"\\rcas.hlsl")){ status_ = L"RCAS compile failed"; return false; }

    // Source/destination formats are carried by the SRV/UAV creation in Process();
    // builtins assume RGBA8. HDR paths pass RGBA16F and use the same numeric layout.

    if (!Build(dev, s)) return false;
    return true;
}

bool ScalerChain::Build(Device& dev, const Settings& s) {
    const std::wstring root = ShaderRoot();
    bool ok = true;
    switch (s.algo) {
    case ScaleAlgo::Nearest:       ok = scale_.Compile(dev.Dev(), root + L"\\nearest.hlsl"); break;
    case ScaleAlgo::Bilinear:      ok = scale_.Compile(dev.Dev(), root + L"\\bilinear.hlsl"); break;
    case ScaleAlgo::SharpBilinear: ok = scale_.Compile(dev.Dev(), root + L"\\sharp_bilinear.hlsl"); break;
    case ScaleAlgo::Lanczos3:      ok = scale_.Compile(dev.Dev(), root + L"\\lanczos3.hlsl"); break;
    case ScaleAlgo::FS1:           ok = scale_.Compile(dev.Dev(), root + L"\\fs1.hlsl"); break;
    case ScaleAlgo::UserShader:
        if (s.userShaderName.empty()) { status_ = L"user shader not selected"; return false; }
        ok = scale_.Compile(dev.Dev(), ShaderDir() + L"\\user\\" + s.userShaderName);
        if (!ok) { status_ = L"user shader failed: " + scale_.LastError(); return false; }
        break;
    }
    if (!ok) { status_ = L"shader compile failed"; return false; }

    algo_ = s.algo;
    userShader_ = s.userShaderName;
    casAmount_ = s.casAmount;
    rcasAmount_ = s.rcasAmount;
    useCas_  = casAmount_  > 0.001f;
    useRcas_ = rcasAmount_ > 0.001f && s.algo != ScaleAlgo::Nearest;
    status_ = L"ok";
    return true;
}

bool ScalerChain::ApplySettings(const Device& dev, const Settings& s,
                                uint32_t srcW, uint32_t srcH, uint32_t dstW, uint32_t dstH) {
    if (srcW != sw_ || srcH != sh_ || dstW != dw_ || dstH != dh_) {
        dev_ = const_cast<Device*>(&dev);
        Settings copy = s;
        Shutdown();
        return Init(const_cast<Device&>(dev), copy, srcW, srcH, dstW, dstH,
                    DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM);
    }
    if (s.algo != algo_ || s.userShaderName != userShader_ ||
        s.casAmount != casAmount_ || s.rcasAmount != rcasAmount_)
        return Build(const_cast<Device&>(dev), s);
    return true;
}

void ScalerChain::HotReloadUserShader(ID3D11Device* dev) {
    if (algo_ != ScaleAlgo::UserShader || userShader_.empty()) return;
    if (scale_.RecompileIfChanged(dev) < 0) return;    // unchanged or failed: keep last good
    LOGI("User shader hot-reloaded: %ls", userShader_.c_str());
}

void ScalerChain::Process(ID3D11Texture2D* src, ID3D11Texture2D* dst) {
    ID3D11DeviceContext* ctx = dev_->Ctx();
    if (!ctx || !scale_.Get()) return;

    srcSrv_.Reset(); dstUav_.Reset();
    D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = fmt_; sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
    ud.Format = fmt_; ud.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    dev_->Dev()->CreateShaderResourceView(src, &sd, srcSrv_.GetAddressOf());
    dev_->Dev()->CreateUnorderedAccessView(dst, &ud, dstUav_.GetAddressOf());

    auto params = [&](uint32_t w, uint32_t h) {
        ShaderParamsCB cb{};
        cb.srcSize[0] = sw_;  cb.srcSize[1] = sh_;
        cb.dstSize[0] = w;    cb.dstSize[1] = h;
        cb.factor = float(w) / float(std::max(1u, sw_));
        cb.sharpen = 0.f;
        return cb;
    };

    ID3D11SamplerState* sams[2] = { samPoint_.Get(), samLinear_.Get() };
    ctx->CSSetSamplers(0, 2, sams);

    // 1) optional CAS pre-sharpen (source-sized)
    if (useCas_) {
        ShaderParamsCB cb = params(sw_, sh_);
        cb.sharpen = casAmount_;
        cas_.Dispatch(ctx, srcSrv_.GetAddressOf(), 1, tmpUav_.Get(), cb, cb_.Get());
        ctx->CSSetSamplers(0, 2, sams);
        scale_.Dispatch(ctx, tmpSrv_.GetAddressOf(), 1, dstUav_.Get(), params(dw_, dh_), cb_.Get());
    } else {
        scale_.Dispatch(ctx, srcSrv_.GetAddressOf(), 1, dstUav_.Get(), params(dw_, dh_), cb_.Get());
    }

    // 2) optional RCAS post-sharpen (destination-sized, in place through tmp_)
    if (useRcas_) {
        ShaderParamsCB cb = params(dw_, dh_);
        cb.sharpen = rcasAmount_;
        // dst -> tmp (RCAS reads dst via SRV), then tmp -> dst
        CPtr<ID3D11ShaderResourceView> dstSrv; CPtr<ID3D11UnorderedAccessView> dstUav2;
        dev_->Dev()->CreateShaderResourceView(dst, &sd, dstSrv.GetAddressOf());
        dev_->Dev()->CreateUnorderedAccessView(tmp_.Get(), &ud, dstUav2.GetAddressOf());
        rcas_.Dispatch(ctx, dstSrv.GetAddressOf(), 1, dstUav2.Get(), cb, cb_.Get());
        ctx->CSSetSamplers(0, 2, sams);
        CPtr<ID3D11ShaderResourceView> tmpSrv2;
        dev_->Dev()->CreateShaderResourceView(tmp_.Get(), &sd, tmpSrv2.GetAddressOf());
        CPtr<ID3D11UnorderedAccessView> dstUav3;
        dev_->Dev()->CreateUnorderedAccessView(dst, &ud, dstUav3.GetAddressOf());
        // plain copy tmp -> dst via bilinear shader semantics: reuse scale shader at 1:1
        scale_.Dispatch(ctx, tmpSrv2.GetAddressOf(), 1, dstUav3.Get(), params(dw_, dh_), cb_.Get());
    }

    srcSrv_.Reset(); dstUav_.Reset();
}

void ScalerChain::Shutdown() {
    srcSrv_.Reset(); dstUav_.Reset(); tmpSrv_.Reset(); tmpUav_.Reset();
    tmp_.Reset(); cb_.Reset(); samPoint_.Reset(); samLinear_.Reset();
    casCs_.Reset(); rcasCs_.Reset();
}

} // namespace fs
