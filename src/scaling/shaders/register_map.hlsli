// FlowScale — shared register map for every scaling/framegen pass.
//
// Contract (docs/SHADERS.md):
//   t0  source texture (RGBA8 or RGBA16F, linear sRGB values)
//   s0  point sampler, s1 linear sampler (clamped addressing)
//   u0  destination texture (write exactly once per thread)
//   b0  ShaderParamsCB { srcSize, dstSize, factor, sharpen }
Texture2D<float4> src     : register(t0);
SamplerState samPoint     : register(s0);
SamplerState samLinear    : register(s1);
RWTexture2D<float4> dst   : register(u0);

cbuffer Params : register(b0)
{
    uint2 srcSize;
    uint2 dstSize;
    float factor;
    float sharpen;
    float2 reserved;
}
