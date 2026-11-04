// FrameGen pass: 2x2 box downsample for the luma pyramid (dstSize = half of source).
//   t0 R8 source, u0 R8 destination, b0 FgParams
Texture2D<float> texSrc  : register(t0);
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
    const int2 src = int2(dt.x, dt.y) * 2;
    const float s = texSrc.Load(int3(src, 0))
                  + texSrc.Load(int3(src + int2(1, 0), 0))
                  + texSrc.Load(int3(src + int2(0, 1), 0))
                  + texSrc.Load(int3(src + int2(1, 1), 0));
    dstY[dt.xy] = s * 0.25;
}
