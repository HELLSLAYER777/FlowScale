// FrameGen pass: coarse optical flow — block matching on the 1/4 luma pyramid.
//
// One thread per flow-grid cell (gridStep source px). SAD over a 3x3 block,
// full search inside ±searchR level px. Output flow is in SOURCE pixels and
// points cur -> prev (where the content came from).
//   t0 prev luma (1/4), t1 cur luma (1/4), u0 R16G16 flow, b0 FgParams
Texture2D<float>  texPrev : register(t0);
Texture2D<float>  texCur  : register(t1);
RWTexture2D<float2> dstF  : register(u0);

cbuffer FgParams : register(b0)
{
    uint2 srcSize;
    uint2 dstSize;      // flow grid size
    float tParam;
    float tau;
    float kOcc;
    float flowClamp;
    int   searchR;
    int   refineR;
    int   gridStep;
    float sharpen;
}

float SadBlock(int2 curC, int2 prevC)   // 3x3 block SAD at quarter res (~12x12 source px)
{
    float sad = 0;
    [unroll]
    for (int j = -1; j <= 1; ++j)
        [unroll]
        for (int i = -1; i <= 1; ++i)
        {
            const int2 d = int2(i, j);
            const int2 pc = clamp(prevC + d, int2(0, 0), int2(srcSize) / 4 - 1);
            const int2 cc = clamp(curC + d, int2(0, 0), int2(srcSize) / 4 - 1);
            sad += abs(texPrev.Load(int3(pc, 0)) - texCur.Load(int3(cc, 0)));
        }
    return sad;
}

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const int2 center = int2(dt.x, dt.y) * (gridStep / 4);   // cell center at quarter res

    float best = 1e9;
    int2 bestD = int2(0, 0);
    [loop]
    for (int dy = -searchR; dy <= searchR; ++dy)
        [loop]
        for (int dx = -searchR; dx <= searchR; ++dx)
        {
            // prev(center + delta) vs cur(center): best delta == flow (cur->prev)
            const float sad = SadBlock(center, center + int2(dx, dy));
            if (sad < best) { best = sad; bestD = int2(dx, dy); }
        }

    // Flow cur->prev in source px: quarter-res offsets scale by 4.
    dstF[dt.xy] = float2(bestD) * 4.0;
}
