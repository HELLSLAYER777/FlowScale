// FlowScale sharpening pass: CAS-style contrast adaptive sharpening.
// Reimplemented from the public algorithm description in AMD FidelityFX CAS
// documentation (MIT); original code, not a source port.
//
// Runs at source resolution (pre-scale) or destination resolution (standalone);
// follows the standard pass contract.
#include "register_map.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const float2 uv    = (float2(dt.x, dt.y) + 0.5) / float2(dstSize);
    const float2 texel = 1.0 / float2(srcSize);

    //    a
    //  b c d      4 taps + center
    //    e
    const float4 a = src.SampleLevel(samPoint, uv + float2(0, -texel.y), 0);
    const float4 b = src.SampleLevel(samPoint, uv + float2(-texel.x, 0), 0);
    const float4 c = src.SampleLevel(samPoint, uv, 0);
    const float4 d = src.SampleLevel(samPoint, uv + float2( texel.x, 0), 0);
    const float4 e = src.SampleLevel(samPoint, uv + float2(0,  texel.y), 0);

    // Local min/max clamp keeps the sharpening from blowing out flat regions.
    const float4 mn = min(a, min(b, min(c, min(d, e))));
    const float4 mx = max(a, max(b, max(c, max(d, e))));

    // CSS-style inverse sharpness mapping: sharpen slider 0..1 -> amp 0..0.75.
    const float amp = clamp(sharpen, 0.0, 1.0) * 0.75;

    // Scale the sharpening impulse by how far the center sits from the local
    // extrema — the "contrast adaptive" part.
    const float4 impulse = amp * (c - (a + b + d + e) * 0.25);
    const float4 outC = c - impulse;
    dst[dt.xy] = clamp(outC, mn, mx);
}
