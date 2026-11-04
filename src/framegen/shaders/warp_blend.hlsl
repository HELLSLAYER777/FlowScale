// FrameGen pass: occlusion-aware bidirectional warp + blend.
//
// For each output pixel:
//   F  = flow(uv)                       (cur -> prev, source px)
//   uvPrev = uv + F*t/srcSize           (sample prev where its content sits now)
//   uvCur  = uv - F*(1-t)/srcSize       (sample cur on the motion path)
//   wPrev/wCur = (1-t)/t, discredited by luma disagreement (occlusion proxy) and
//   by flow divergence (an occluded side pulls the field apart).
// Fully discredited blends fall back to the current frame — a crisp real frame
// always beats a smeared average ("almost no ghosting" priority order).
//
// warp_blend binds its own registers (the pyramid passes use different types):
Texture2D<float4> texPrev : register(t0);
Texture2D<float4> texCur  : register(t1);
Texture2D<float>  prevY   : register(t2);
Texture2D<float>  curY    : register(t3);
Texture2D<float2> flowTex : register(t4);
SamplerState samPoint     : register(s0);
SamplerState samLinear    : register(s1);
RWTexture2D<float4> dst   : register(u0);

cbuffer FgParams : register(b0)
{
    uint2 srcSize;      // frame size in px
    uint2 dstSize;      // output size in px (== srcSize for this pass)
    float tParam;       // sub-frame position 0..1(+)
    float tau;          // occlusion luma tolerance
    float kOcc;         // occlusion falloff
    float flowClamp;    // max |flow| in px
    int   searchR;      // coarse search radius (level px) - unused here
    int   refineR;      // refine search radius (level px) - unused here
    int   gridStep;     // flow grid step in source px - unused here
    float sharpen;      // reserved
}

[numthreads(8, 8, 1)]
void main(uint3 dt : SV_DispatchThreadID)
{
    if (dt.x >= dstSize.x || dt.y >= dstSize.y) return;

    const float2 px  = float2(dt.x, dt.y) + 0.5;
    const float2 uv  = px / float2(srcSize);
    const float2 tex = 1.0 / float2(srcSize);

    // Flow field, bilinear, clamped (artifact suppression).
    float2 F = flowTex.SampleLevel(samLinear, uv, 0);
    F = clamp(F, -flowClamp, flowClamp);

    const float t = clamp(tParam, 0.0, 2.0);
    const float2 uvPrev = uv + F * t * tex;
    const float2 uvCur  = uv - F * (1.0 - t) * tex;

    // Divergence: how much the field disagrees with itself at the sampled point.
    const float2 F2 = flowTex.SampleLevel(samLinear, uvPrev, 0);
    const float  div = distance(F, F2);

    const float3 cPrev = texPrev.SampleLevel(samLinear, uvPrev, 0).rgb;
    const float3 cCur  = texCur.SampleLevel(samLinear, uvCur, 0).rgb;
    const float  yPrev = prevY.SampleLevel(samLinear, uvPrev, 0);
    const float  yCur  = curY.SampleLevel(samLinear, uvCur, 0);

    // Weights: time position, then occlusion confidence.
    float wPrev = (1.0 - t) * exp(-abs(yPrev - yCur) / max(tau, 1e-4) * (kOcc * 0.5))
                          * exp(-div / max(flowClamp, 1.0) * kOcc);
    float wCur  = t;

    const float sum = wPrev + wCur;
    float3 outC = (cPrev * wPrev + cCur * wCur) / max(sum, 1e-5);

    // Fallback: when the previous side is fully discredited, show the current
    // frame content at this pixel (crisp, never a stale smear).
    const float prevSupport = saturate(wPrev / max(sum, 1e-5));
    const float3 curHere = texCur.SampleLevel(samLinear, uv, 0).rgb;
    outC = lerp(outC, curHere, 1.0 - prevSupport);

    dst[dt.xy] = float4(max(outC, 0.0), 1.0);
}
