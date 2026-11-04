// FlowScale scaling pass: Lanczos-3 (2-lobe sinc window), 6x6 separable taps.
//
// Negative lobes are preserved but the final result is clamped to the local
// input min/max range to kill ringing halos around hard edges.
#include "register_map.hlsli"

float Lanczos2(float x)
{
    x = abs(x);
    if (x < 1e-5) return 1.0;
    if (x >= 2.0) return 0.0;
    const float px = 3.14159265358979 * x;
    return 2.0 * sin(px) * sin(px * 0.5) / (px * px);
}

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const float2 dstPx = float2(dt.x, dt.y) + 0.5;
    const float2 uv    = dstPx / float2(dstSize);
    const float2 srcPx = uv * float2(srcSize) - 0.5;      // source-space position
    const float2 base  = floor(srcPx);

    float4 acc = 0, mn = 1e9, mx = -1e9;
    float wsum = 0;
    [unroll]
    for (int j = -2; j <= 3; ++j)
    {
        const float wy = Lanczos2((srcPx.y - (base.y + j)) );
        [unroll]
        for (int i = -2; i <= 3; ++i)
        {
            const float2 p = clamp(base + float2(i, j) + 0.5, 0.5, float2(srcSize) - 0.5);
            const float  w = wy * Lanczos2(srcPx.x - (base.x + i));
            float4 s = src.SampleLevel(samPoint, p / float2(srcSize), 0);
            acc += s * w;
            wsum += w;
            mn = min(mn, s);
            mx = max(mx, s);
        }
    }

    float4 outC = acc / wsum;
    dst[dt.xy] = clamp(outC, mn, mx);
}
