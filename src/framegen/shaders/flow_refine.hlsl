// FrameGen pass: flow refinement on the 1/2 luma pyramid.
//
// Seeds from the coarse field, searches ±refineR level px around it, fits a
// parabola through the SAD neighborhood for sub-pixel precision (identical
// formula to fs::flow::ParabolaSubPixel in the tested CPU reference).
//   t0 prev luma (1/2), t1 cur luma (1/2), t2 coarse flow, u0 R16G16 flow
Texture2D<float>  texPrev : register(t0);
Texture2D<float>  texCur  : register(t1);
Texture2D<float2> texFlowA: register(t2);
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

float SadBlockHalf(int2 curC, int2 prevC)   // 4x4 block SAD at half res (~8x8 source px)
{
    float sad = 0;
    [unroll]
    for (int j = -2; j < 2; ++j)
        [unroll]
        for (int i = -2; i < 2; ++i)
        {
            const int2 d = int2(i, j);
            const int2 pc = clamp(prevC + d, int2(0, 0), int2(srcSize) / 2 - 1);
            const int2 cc = clamp(curC + d, int2(0, 0), int2(srcSize) / 2 - 1);
            sad += abs(texPrev.Load(int3(pc, 0)) - texCur.Load(int3(cc, 0)));
        }
    return sad;
}

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const int2 center = int2(dt.x, dt.y) * (gridStep / 2);   // cell center at half res

    // Prior from the coarse field (nearest cell; grid resolutions match here).
    const float2 priorL = texFlowA.Load(int3(dt.x, dt.y, 0)) * 0.5;   // source px -> half-res px

    float best = 1e9;
    int2 bestD = int2(0, 0);
    [loop]
    for (int dy = -refineR; dy <= refineR; ++dy)
        [loop]
        for (int dx = -refineR; dx <= refineR; ++dx)
        {
            const int2 d = int2(round(priorL.x), round(priorL.y)) + int2(dx, dy);
            const float sad = SadBlockHalf(center, center + d);
            if (sad < best) { best = sad; bestD = d; }
        }

    // Sub-pixel parabola through the SAD neighborhood.
    const float s1 = SadBlockHalf(center, center + bestD + int2(-1, 0));
    const float s0 = best;
    const float s2 = SadBlockHalf(center, center + bestD + int2( 1, 0));
    float subX = 0;
    {
        const float den = s1 - 2 * s0 + s2;
        if (den > 1e-6) subX = clamp(0.5 * (s1 - s2) / den, -0.5, 0.5);
    }
    const float t1 = SadBlockHalf(center, center + bestD + int2(0, -1));
    const float t2 = SadBlockHalf(center, center + bestD + int2(0,  1));
    float subY = 0;
    {
        const float den = t1 - 2 * s0 + t2;
        if (den > 1e-6) subY = clamp(0.5 * (t1 - t2) / den, -0.5, 0.5);
    }

    dstF[dt.xy] = (float2(bestD) + float2(subX, subY)) * 2.0;   // back to source px
}
