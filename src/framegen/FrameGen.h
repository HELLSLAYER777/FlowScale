// FlowScale — frame generation: flow-based interpolation engine.
//
// Pipeline on every new real frame:
//   rgba_to_y  -> yA (full)  -> downsample2x -> yB (1/2) -> downsample2x -> yC (1/4)
//   flow_coarse(Yc_prev, Yc_cur) -> flowA   (search, 1/4-res SAD, source-px vectors)
//   flow_refine(Yb_prev, Yb_cur, flowA) -> flowB (+ parabola sub-pixel)
// On generated ticks:
//   warp_blend(prev, cur, Yfull both, flowB, t) -> generated RGBA (source resolution)
// The scaler chain treats generated frames exactly like real ones afterwards.
#pragma once
#include <cmath>
#include "gfx/CShader.h"
#include "core/Settings.h"

namespace fs {

// Shared HLSL cbuffer layout (src/framegen/shaders/register_map.hlsli).
struct FgCb {
    uint32_t srcSize[2];
    uint32_t dstSize[2];
    float    tParam;
    float    tau;
    float    kOcc;
    float    flowClamp;
    int32_t  searchR;
    int32_t  refineR;
    int32_t  gridStep;
    float    sharpen;
};
static_assert(sizeof(FgCb) == 48, "cbuffer layout drift");

inline void UploadCb(ID3D11DeviceContext* ctx, ID3D11Buffer* cb, const FgCb& v) {
    D3D11_MAPPED_SUBRESOURCE m{};
    if (SUCCEEDED(ctx->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        memcpy(m.pData, &v, sizeof(v));
        ctx->Unmap(cb, 0);
    }
}

class FrameGen {
public:
    // Compiles shaders and allocates textures for a source size.
    bool Init(Device& dev, uint32_t srcW, uint32_t srcH, FgQuality q, float artifactSup);
    void Shutdown();

    // Feed a new real frame (engine-owned texture); internally rotates prev<-cur.
    // Runs on the present thread with the shared context.
    void OnNewRealFrame(ID3D11Texture2D* rgba);

    // True once two real frames are in history (generation becomes possible).
    bool Ready() const { return hasPrev_ && hasCur_; }

    // Generate the interpolated frame at sub-frame position t; the returned
    // texture stays valid until the next Generate()/OnNewRealFrame() call.
    ID3D11Texture2D* Generate(double t);

    // Applies quality/artifact-suppression changes live.
    void ApplySettings(FgQuality q, float artifactSup);

    uint32_t Width()  const { return w_; }
    uint32_t Height() const { return h_; }

private:
    struct YChain {  // rgba + luma pyramid (full / 1/2 / 1/4)
        CPtr<ID3D11Texture2D> rgba;
        CPtr<ID3D11Texture2D> full, half, quart;
        CPtr<ID3D11ShaderResourceView> rgbaSrv, fullSrv, halfSrv, quartSrv;
        CPtr<ID3D11UnorderedAccessView> fullUav, halfUav, quartUav;
    };
    void BuildYChain(YChain& y);
    void ComputeYChain(YChain& y);          // rgba -> full -> half -> quart
    void RecomputeFlow();                   // coarse (+ refine) after each real frame
    float FlowClampPx() const { return clampFrac_ * sqrtf(float(w_ * w_ + h_ * h_)); }

    Device* dev_ = nullptr;
    uint32_t w_ = 0, h_ = 0, gw_ = 0, gh_ = 0;

    YChain a_, b_;                          // a = previous real frame, b = latest
    bool hasPrev_ = false, hasCur_ = false;

    CPtr<ID3D11Texture2D> flowA_, flowB_;
    CPtr<ID3D11ShaderResourceView> flowASrv_, flowBSrv_;
    CPtr<ID3D11UnorderedAccessView> flowAUav_, flowBUav_;
    CPtr<ID3D11Texture2D> out_;
    CPtr<ID3D11UnorderedAccessView> outUav_;
    CPtr<ID3D11Buffer> cb_;
    CPtr<ID3D11SamplerState> samPoint_, samLinear_;

    CShader toY_, down2x_, coarse_, refine_, blend_;

    // Tunables (quality preset + artifact suppression slider).
    int   coarseR_ = 12, refineR_ = 5;
    int   gridStep_ = 8;
    bool  useRefine_ = true;
    float tau_ = 0.10f, kOcc_ = 14.0f, clampFrac_ = 0.12f;
};

} // namespace fs
