// FlowScale user shader template — copy this file, rename, edit, press F5.
// Full contract: docs/SHADERS.md
#include "../scaling/register_map.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;
    const float2 uv = (float2(dt.x, dt.y) + 0.5) / float2(dstSize);
    float4 c = src.SampleLevel(samLinear, uv, 0);

    // --- your algorithm here ---

    dst[dt.xy] = c;
}
