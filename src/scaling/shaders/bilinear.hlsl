// FlowScale scaling pass: plain bilinear.
#include "register_map.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;
    const float2 uv = (float2(dt.x, dt.y) + 0.5) / float2(dstSize);
    dst[dt.xy] = src.SampleLevel(samLinear, uv, 0);
}
