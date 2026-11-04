// FlowScale scaling pass: sharp bilinear (pixel-art friendly).
//
// Bilinear upscaling blurs integer-art at factors > 2 because most destination
// pixels land between source texels. The fix: snap the sample coordinate to the
// nearest source texel center (restoring the crisp grid), then bilinear only the
// boundary pixels, then apply a mild center-weighted sharpen.
#include "register_map.hlsli"

float4 Sharpen4(float4 c, float4 aL, float4 aR, float4 aT, float4 aB, float amount)
{
    return c + amount * (c * 4.0 - aL - aR - aT - aB);
}

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const float2 uv = (float2(dt.x, dt.y) + 0.5) / float2(dstSize);
    const float2 texel = 1.0 / float2(srcSize);

    // Snap to source texel grid when the factor is large enough for the grid to matter.
    float2 coord = uv;
    if (factor >= 2.0)
        coord = (floor(uv * float2(srcSize)) + 0.5) * texel;

    float4 c = src.SampleLevel(samLinear, coord, 0);

    const float amt = 0.25 * saturate(sharpen * 2.0);
    if (amt > 0.0)
    {
        float4 aL = src.SampleLevel(samLinear, coord + float2(-texel.x, 0), 0);
        float4 aR = src.SampleLevel(samLinear, coord + float2( texel.x, 0), 0);
        float4 aT = src.SampleLevel(samLinear, coord + float2(0, -texel.y), 0);
        float4 aB = src.SampleLevel(samLinear, coord + float2(0,  texel.y), 0);
        c = Sharpen4(c, aL, aR, aT, aB, amt);
    }
    dst[dt.xy] = max(c, 0.0);
}
