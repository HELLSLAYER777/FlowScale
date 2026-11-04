// FlowScale scaling pass: FS1 — FlowScale's own edge-adaptive scaler.
//
// Original design (not a port of AMD EASU or NVIDIA NIS): upscale with bilinear,
// then re-inject high-frequency content along the local gradient normal. The
// gradient magnitude gates the effect, so flat regions stay clean and edges
// regain crispness lost to bilinear averaging.
#include "register_map.hlsli"

float Luma(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const float2 uv    = (float2(dt.x, dt.y) + 0.5) / float2(dstSize);
    const float2 texel = 1.0 / float2(srcSize);

    float4 c = src.SampleLevel(samLinear, uv, 0);

    // Local luma gradient at the source position.
    const float4 l = src.SampleLevel(samLinear, uv + float2(-texel.x, 0), 0);
    const float4 r = src.SampleLevel(samLinear, uv + float2( texel.x, 0), 0);
    const float4 t = src.SampleLevel(samLinear, uv + float2(0, -texel.y), 0);
    const float4 b = src.SampleLevel(samLinear, uv + float2(0,  texel.y), 0);

    const float gx = Luma(r.rgb) - Luma(l.rgb);
    const float gy = Luma(b.rgb) - Luma(t.rgb);
    const float grad = sqrt(gx * gx + gy * gy);

    // Edge normal = gradient direction; sample across the normal on both sides.
    float2 n = grad > 1e-4 ? normalize(float2(gx, gy)) : float2(0, 0);
    const float4 across = src.SampleLevel(samLinear, uv + n * texel * 0.75, 0)
                        + src.SampleLevel(samLinear, uv - n * texel * 0.75, 0);

    // Edge boost: proportional to local contrast, limited by user sharpen slider.
    const float edge = saturate(grad * 3.0) * saturate(sharpen * 1.5 + 0.25);
    dst[dt.xy] = max(c + edge * (c - across * 0.5), 0.0);
}
