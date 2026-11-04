#include "framegen/FrameGen.h"
#include "util/Common.h"
#include "util/Log.h"
#include <algorithm>
#include <cmath>

namespace fs {

static std::wstring FgShaderDir() { return ShaderDir() + L"\\framegen"; }

void FrameGen::BuildYChain(YChain& y) {
    auto makeTex = [&](uint32_t w, uint32_t h, DXGI_FORMAT fmt, UINT bind, CPtr<ID3D11Texture2D>& tex) {
        D3D11_TEXTURE2D_DESC td{};
        td.Width = std::max(1u, w); td.Height = std::max(1u, h);
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = fmt; td.SampleDesc.Count = 1;
        td.BindFlags = bind;
        ThrowIfFailed(dev_->Dev()->CreateTexture2D(&td, nullptr, tex.GetAddressOf()), "fg tex");
    };
    auto makeSrv = [&](ID3D11Texture2D* tex, DXGI_FORMAT fmt, CPtr<ID3D11ShaderResourceView>& srv) {
        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = fmt; sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; sd.Texture2D.MipLevels = 1;
        ThrowIfFailed(dev_->Dev()->CreateShaderResourceView(tex, &sd, srv.GetAddressOf()), "fg srv");
    };
    auto makeUav = [&](ID3D11Texture2D* tex, DXGI_FORMAT fmt, CPtr<ID3D11UnorderedAccessView>& uav) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.Format = fmt; ud.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        ThrowIfFailed(dev_->Dev()->CreateUnorderedAccessView(tex, &ud, uav.GetAddressOf()), "fg uav");
    };

    makeTex(w_, h_, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_BIND_SHADER_RESOURCE, y.rgba);
    makeSrv(y.rgba.Get(), DXGI_FORMAT_B8G8R8A8_UNORM, y.rgbaSrv);

    const UINT lumaBind = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    makeTex(w_, h_, DXGI_FORMAT_R8_UNORM, lumaBind, y.full);
    makeTex(w_ / 2, h_ / 2, DXGI_FORMAT_R8_UNORM, lumaBind, y.half);
    makeTex(w_ / 4, h_ / 4, DXGI_FORMAT_R8_UNORM, lumaBind, y.quart);
    makeSrv(y.full.Get(), DXGI_FORMAT_R8_UNORM, y.fullSrv);
    makeSrv(y.half.Get(), DXGI_FORMAT_R8_UNORM, y.halfSrv);
    makeSrv(y.quart.Get(), DXGI_FORMAT_R8_UNORM, y.quartSrv);
    makeUav(y.full.Get(), DXGI_FORMAT_R8_UNORM, y.fullUav);
    makeUav(y.half.Get(), DXGI_FORMAT_R8_UNORM, y.halfUav);
    makeUav(y.quart.Get(), DXGI_FORMAT_R8_UNORM, y.quartUav);
}

void FrameGen::ComputeYChain(YChain& y) {
    ID3D11DeviceContext* ctx = dev_->Ctx();
    ID3D11UnorderedAccessView* nullUav = nullptr;
    ID3D11ShaderResourceView* nullSrv[1] = { nullptr };

    auto dispatchLuma = [&](CShader& cs, ID3D11ShaderResourceView* srv,
                            CPtr<ID3D11UnorderedAccessView>& uav, uint32_t w, uint32_t h) {
        FgCb p{};
        p.srcSize[0] = w_; p.srcSize[1] = h_;
        p.dstSize[0] = w;  p.dstSize[1] = h;
        UploadCb(ctx, cb_.Get(), p);
        ctx->CSSetConstantBuffers(0, 1, cb_.GetAddressOf());
        ctx->CSSetShaderResources(0, 1, &srv);
        ctx->CSSetUnorderedAccessViews(0, 1, uav.GetAddressOf(), nullptr);
        ctx->CSSetShader(cs.Get(), nullptr, 0);
        ctx->Dispatch((w + 7) / 8, (h + 7) / 8, 1);
        ctx->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
        ctx->CSSetShaderResources(0, 1, nullSrv);
        ctx->CSSetShader(nullptr, nullptr, 0);
    };

    dispatchLuma(toY_,   y.rgbaSrv.Get(), y.fullUav,  w_,     h_);
    dispatchLuma(down2x_, y.fullSrv.Get(), y.halfUav, w_ / 2, h_ / 2);
    dispatchLuma(down2x_, y.halfSrv.Get(), y.quartUav, w_ / 4, h_ / 4);
}

void FrameGen::RecomputeFlow() {
    ID3D11DeviceContext* ctx = dev_->Ctx();
    ID3D11UnorderedAccessView* nullUav = nullptr;
    ID3D11ShaderResourceView* nullSrv[3] = {};

    auto baseParams = [&](int searchR, int refineR) {
        FgCb p{};
        p.srcSize[0] = w_; p.srcSize[1] = h_;
        p.dstSize[0] = gw_; p.dstSize[1] = gh_;
        p.searchR = searchR; p.refineR = refineR; p.gridStep = gridStep_;
        p.flowClamp = FlowClampPx();
        return p;
    };

    // Coarse: quart prev/cur -> flowA.
    {
        UploadCb(ctx, cb_.Get(), baseParams(coarseR_, refineR_));
        ID3D11ShaderResourceView* srvs[3] = { a_.quartSrv.Get(), b_.quartSrv.Get(), nullptr };
        ctx->CSSetConstantBuffers(0, 1, cb_.GetAddressOf());
        ctx->CSSetShaderResources(0, 2, srvs);
        ctx->CSSetUnorderedAccessViews(0, 1, flowAUav_.GetAddressOf(), nullptr);
        ctx->CSSetShader(coarse_.Get(), nullptr, 0);
        ctx->Dispatch((gw_ + 7) / 8, (gh_ + 7) / 8, 1);
        ctx->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
        ctx->CSSetShaderResources(0, 2, nullSrv);
    }

    // Refine: half prev/cur + flowA -> flowB.
    if (useRefine_) {
        UploadCb(ctx, cb_.Get(), baseParams(coarseR_, refineR_));
        ID3D11ShaderResourceView* srvs[3] = { a_.halfSrv.Get(), b_.halfSrv.Get(), flowASrv_.Get() };
        ctx->CSSetConstantBuffers(0, 1, cb_.GetAddressOf());
        ctx->CSSetShaderResources(0, 3, srvs);
        ctx->CSSetUnorderedAccessViews(0, 1, flowBUav_.GetAddressOf(), nullptr);
        ctx->CSSetShader(refine_.Get(), nullptr, 0);
        ctx->Dispatch((gw_ + 7) / 8, (gh_ + 7) / 8, 1);
        ctx->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
        ctx->CSSetShaderResources(0, 3, nullSrv);
    } else {
        ctx->CopyResource(flowB_.Get(), flowA_.Get());
    }
    ctx->CSSetShader(nullptr, nullptr, 0);
}

bool FrameGen::Init(Device& dev, uint32_t srcW, uint32_t srcH, FgQuality q, float artifactSup) {
    dev_ = &dev;
    w_ = srcW; h_ = srcH;
    gw_ = std::max(1u, (w_ + gridStep_ - 1) / gridStep_);
    gh_ = std::max(1u, (h_ + gridStep_ - 1) / gridStep_);

    const std::wstring dir = FgShaderDir();
    if (!toY_.Compile(dev.Dev(), dir + L"\\rgba_to_y.hlsl"))     return false;
    if (!down2x_.Compile(dev.Dev(), dir + L"\\downsample2x.hlsl")) return false;
    if (!coarse_.Compile(dev.Dev(), dir + L"\\flow_coarse.hlsl")) return false;
    if (!refine_.Compile(dev.Dev(), dir + L"\\flow_refine.hlsl")) return false;
    if (!blend_.Compile(dev.Dev(), dir + L"\\warp_blend.hlsl"))   return false;

    cb_        = dev.CreateConstantBuffer(sizeof(FgCb));
    samPoint_  = dev.CreateSampler(false);
    samLinear_ = dev.CreateSampler(true);

    BuildYChain(a_);
    BuildYChain(b_);

    auto mkFlow = [&](CPtr<ID3D11Texture2D>& tex, CPtr<ID3D11ShaderResourceView>& srv,
                      CPtr<ID3D11UnorderedAccessView>& uav) {
        D3D11_TEXTURE2D_DESC td{};
        td.Width = gw_; td.Height = gh_; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R16G16_FLOAT; td.SampleDesc.Count = 1;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        ThrowIfFailed(dev.Dev()->CreateTexture2D(&td, nullptr, tex.GetAddressOf()), "flow tex");
        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = DXGI_FORMAT_R16G16_FLOAT; sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = 1;
        D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.Format = DXGI_FORMAT_R16G16_FLOAT; ud.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        ThrowIfFailed(dev.Dev()->CreateShaderResourceView(tex.Get(), &sd, srv.GetAddressOf()), "flow srv");
        ThrowIfFailed(dev.Dev()->CreateUnorderedAccessView(tex.Get(), &ud, uav.GetAddressOf()), "flow uav");
    };
    mkFlow(flowA_, flowASrv_, flowAUav_);
    mkFlow(flowB_, flowBSrv_, flowBUav_);

    // Generated output at source resolution (the scaler chain scales it after).
    D3D11_TEXTURE2D_DESC od{};
    od.Width = w_; od.Height = h_; od.MipLevels = 1; od.ArraySize = 1;
    od.Format = DXGI_FORMAT_B8G8R8A8_UNORM; od.SampleDesc.Count = 1;
    od.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    ThrowIfFailed(dev.Dev()->CreateTexture2D(&od, nullptr, out_.GetAddressOf()), "fg out tex");
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.Format = DXGI_FORMAT_B8G8R8A8_UNORM; ud.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        ThrowIfFailed(dev.Dev()->CreateUnorderedAccessView(out_.Get(), &ud,
                                                          outUav_.GetAddressOf()), "fg out uav");
    }

    hasPrev_ = hasCur_ = false;
    ApplySettings(q, artifactSup);
    LOGI("FrameGen ready (%ux%u, flow grid %ux%u)", w_, h_, gw_, gh_);
    return true;
}

void FrameGen::ApplySettings(FgQuality q, float artifactSup) {
    switch (q) {
    case FgQuality::Performance:
        coarseR_ = 10; refineR_ = 0; useRefine_ = false; gridStep_ = 16; break;
    case FgQuality::Balanced:
        coarseR_ = 12; refineR_ = 5; useRefine_ = true; gridStep_ = 8; break;
    case FgQuality::Quality:
        coarseR_ = 16; refineR_ = 6; useRefine_ = true; gridStep_ = 8; break;
    }
    // Artifact suppression slider: stronger = smaller flow clamp + harder occlusion falloff.
    clampFrac_ = 0.04f + 0.16f * (1.0f - std::clamp(artifactSup, 0.0f, 1.0f));
    kOcc_ = 8.0f + 12.0f * std::clamp(artifactSup, 0.0f, 1.0f);
}

void FrameGen::OnNewRealFrame(ID3D11Texture2D* rgba) {
    // Rotate: prev <- cur, cur <- new frame; recompute flow for the new segment.
    if (hasCur_) { std::swap(a_, b_); hasPrev_ = true; }
    dev_->Ctx()->CopyResource(b_.rgba.Get(), rgba);
    ComputeYChain(b_);
    hasCur_ = true;
    RecomputeFlow();
}

ID3D11Texture2D* FrameGen::Generate(double t) {
    if (!Ready()) return b_.rgba.Get();

    ID3D11DeviceContext* ctx = dev_->Ctx();
    FgCb p{};
    p.srcSize[0] = w_; p.srcSize[1] = h_;
    p.dstSize[0] = w_; p.dstSize[1] = h_;
    p.tParam = static_cast<float>(t);
    p.tau = tau_;
    p.kOcc = kOcc_;
    p.flowClamp = FlowClampPx();
    UploadCb(ctx, cb_.Get(), p);

    ID3D11ShaderResourceView* srvs[5] = {
        a_.rgbaSrv.Get(), b_.rgbaSrv.Get(), a_.fullSrv.Get(), b_.fullSrv.Get(), flowBSrv_.Get()
    };
    ID3D11UnorderedAccessView* nullUav = nullptr;
    ID3D11SamplerState* sams[2] = { samPoint_.Get(), samLinear_.Get() };

    ctx->CSSetConstantBuffers(0, 1, cb_.GetAddressOf());
    ctx->CSSetShaderResources(0, 5, srvs);
    ctx->CSSetSamplers(0, 2, sams);
    ctx->CSSetUnorderedAccessViews(0, 1, outUav_.GetAddressOf(), nullptr);
    ctx->CSSetShader(blend_.Get(), nullptr, 0);
    ctx->Dispatch((w_ + 7) / 8, (h_ + 7) / 8, 1);
    ctx->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
    ID3D11ShaderResourceView* nullSrv[5] = {};
    ctx->CSSetShaderResources(0, 5, nullSrv);
    ctx->CSSetShader(nullptr, nullptr, 0);
    return out_.Get();
}

void FrameGen::Shutdown() {
    a_ = {}; b_ = {};
    flowA_.Reset(); flowB_.Reset(); flowASrv_.Reset(); flowBSrv_.Reset();
    flowAUav_.Reset(); flowBUav_.Reset();
    out_.Reset(); outUav_.Reset(); cb_.Reset(); samPoint_.Reset(); samLinear_.Reset();
    dev_ = nullptr;
    w_ = h_ = gw_ = gh_ = 0;
    hasPrev_ = hasCur_ = false;
}

} // namespace fs
