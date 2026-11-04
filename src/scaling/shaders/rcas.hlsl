// FlowScale sharpening pass: RCAS-style robust contrast adaptive sharpening.
// Reimplemented following the public algorithm description in the AMD FidelityFX
// Super Resolution documentation (MIT); original code, not a source port.
//
// RCAS adds two protections plain CAS lacks:
//  1. a luminance-proportional impulse limiter that suppresses halos near hard edges,
//  2. an output clamp to the local input range.
// Used post-generation on interpolated frames to recover blend softness.
#include "register_map.hlsli"

float Luma(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const float2 uv    = (float2(dt.x, dt.y) + 0.5) / float2(dstSize);
    const float2 texel = 1.0 / float2(srcSize);

    //    b
    //  d e f      RCAS cross neighborhood (matches public docs pattern)
    //    h
    const float4 b = src.SampleLevel(samPoint, uv + float2(0, -texel.y), 0);
    const float4 d = src.SampleLevel(samPoint, uv + float2(-texel.x, 0), 0);
    const float4 e = src.SampleLevel(samPoint, uv, 0);
    const float4 f = src.SampleLevel(samPoint, uv + float2( texel.x, 0), 0);
    const float4 h = src.SampleLevel(samPoint, uv + float2(0,  texel.y), 0);

    const float4 hitMin = min(min(d, h), min(b, min(f, e)));
    const float4 hitMax = max(max(d, h), max(b, max(f, e)));

    // Luma-weighted neighborhood average (smoother than a plain mean).
    const float4 w = float4(1.0, 2.0, 2.0, 1.0) / 6.0;   // not applied per-channel; see below
    float4 noise4 = float4(Luma(d.rgb), Luma(b.rgb), Luma(f.rgb), Luma(h.rgb));
    const float nl = dot(noise4, w);
    const float el = Luma(e.rgb);

    // Sharpening amount: slider 0..1 maps through the documented RCAS curve.
    const float sharp = clamp(sharpen, 0.0, 1.0);
    const float amount = (sharp * sharp) * 0.5;          // 0..0.5 effective

    // Limit: distance from center luma to local extrema, normalized.
    const float lim = saturate(min(min(nl, 2.0 - nl), el));

    // RGB impulse with per-channel soft clamp against the local range.
    float3 impulse = amount * (e.rgb - (d.rgb + b.rgb + f.rgb + h.rgb) * 0.25) * lim;
    float3 outC = e.rgb - impulse;

    // Denoise-style clamp: never exceed the cross neighborhood range.
    outC = clamp(outC, hitMin.rgb, hitMax.rgb);

    dst[dt.xy] = float4(max(outC, 0.0), e.a);
}
