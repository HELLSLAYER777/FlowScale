# User shaders — write a scaler in 30 lines

Any `.hlsl` compute shader dropped into `bin/shaders/user/` becomes selectable in the UI and
hot-reloads on **F5** while FlowScale is running. Shipped algorithms are just shaders that
follow the same contract.

## The contract

```hlsl
// my_scaler.hlsl
Texture2D<float4> src        : register(t0);   // source frame (RGBA8 or RGBA16F)
RWTexture2D<float4> dst      : register(u0);   // destination frame
SamplerState        samPoint : register(s0);
SamplerState        samLinear: register(s1);

cbuffer Params : register(b0)
{
    uint2 srcSize;    // source resolution in px
    uint2 dstSize;    // destination resolution in px
    float factor;     // dstSize.x / srcSize.x
    float sharpen;    // 0..1 user slider (CAS/RCAS amount)
    float pad0, pad1; // reserved
};

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;
    float2 uv = (float2(dt.x, dt.y) + 0.5) / float2(dstSize);
    float4 c  = src.SampleLevel(samLinear, uv, 0);
    dst[dt.xy] = c;   // replace with your algorithm
}
```

Rules:

* entry point is `main`, `[numthreads(8,8,1)]`, one output pixel per thread;
* write **linear sRGB** float4; the engine handles format conversion;
* no headers beyond the standard HLSL includes — the engine compiles with `D3DCompile`
  (shader model 5.0) at load time and shows compile errors in the UI status line.

## Ideas to drop in

* Anime4K restore passes (MIT) — adapt the uniforms to the cbuffer above
* xBR / xBRZ HLSL ports for pixel-art scaling
* NVIDIA NIS (`NISShader.hlsl`, MIT) — its coefficients block maps to `Params`
* Your own trained de-blur / dithering networks exported to compute

## Hot reload details

F5 re-scans the folder, recompiles files whose mtime changed, and swaps the pipeline without
dropping the capture session. Compile failures keep the last good shader and surface the
compiler log in the status bar.
