// FrameGen pass: RGBA -> luma (full resolution).
// Standalone bindings (each FrameGen pass declares exactly what it reads):
//   t0 rgba source, u0 R8 luma destination, b0 FgParams
Texture2D<float4> texSrc : register(t0);
RWTexture2D<float> dstY  : register(u0);

cbuffer FgParams : register(b0)
{
    uint2 srcSize;
    uint2 dstSize;
    float tParam;
    float tau;
    float kOcc;
    float flowClamp;
    int   searchR;
    int   refineR;
    int   gridStep;
    float sharpen;
}

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;
    const float3 c = texSrc.Load(int3(dt.x, dt.y, 0)).rgb;
    dstY[dt.xy] = dot(c, float3(0.2126, 0.7152, 0.0722));
}
