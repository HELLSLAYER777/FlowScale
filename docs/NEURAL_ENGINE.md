# The neural engine — what is possible, what is not, and how we get there

## The DLSS question, answered honestly

DLSS (including "DLSS 4.x" and anything NVIDIA ships next) is proprietary: closed weights,
closed runtime, NDA-gated SDK, hard-wired to NVIDIA drivers and to per-game integration.
There is **no legal or technical mechanism to embed DLSS in FlowScale** (or in any third-party
tool). Any project claiming to "bundle DLSS" is misusing driver components or lying.

What *is* achievable — and what FlowScale implements — is the same **class** of techniques
using open components:

| Capability you asked for | How FlowScale delivers it |
|---|---|
| DLSS-class upscaling | NIS-class spatial upscaling (the open NVIDIA Image Scaling concept) + our trainable neural upscaler (below) |
| Neural rendering / "DLSS 5" ideas | Temporal-accumulation upscaler is on the v1.5+ roadmap (open research: FSR3-style optical-flow-anchored temporal accumulation). The ONNX slot already accepts temporal models. |
| "AI generating frames with learning" | `train_interpolator.py` trains a RIFE-style interpolation network on your footage → `neural_fg.onnx` → the frame generator uses it instead of (or fused with) the flow estimator |
| Almost no ghosting | Neural interpolators trained on your content have near-zero ghosting on-distribution; the flow path fights ghosting with occlusion masks + flow clamping (below) |
| Low latency | Max frame latency 1, waitable swapchain pacing, zero-copy capture, optional predictive extrapolation, HUD that measures the real number |

## Architecture

```
Engine (present thread)
   │  real frames + timestamps
   ▼
FrameGen ── flow path (always available, any GPU) ──► blended frame
   │
   └── neural path (if FS_WITH_NEURAL and bin/models/neural_fg.onnx)
         NeuralEngine (ONNX Runtime): inputs [prev RGB, cur RGB, t] → output [interpolated RGB]
```

* `neural_upscale.onnx` — contract: input `src` float16/32 `1×3×H×W` (NCHW, RGB 0..1) →
  output `dst` `1×3×(sH)×(sW)`. Dynamic H/W supported; engine letterboxes to multiples of 8.
* `neural_fg.onnx` — contract: inputs `prev` `1×3×H×W`, `cur` `1×3×H×W`, `t` scalar →
  output `out` `1×3×H×W`. Same resolution both inputs (we scale first, interpolate second).

Execution: ONNX Runtime session created lazily on first use; EP selection order =
DirectML (if `onnxruntime_providers_directml.dll` present) → CPU. DirectML runs on the same
adapter as the D3D11 pipeline; frames cross via staging textures. FP16 is used when the model
was exported with fp16 IO.

## Anti-ghosting techniques (flow path)

Ghosting in flow-based generation has three sources; each has a counter-measure:

1. **Occlusion** (content appears/disappears) → `warp_blend.hlsl` compares the warped luma of
   both real frames at every output pixel; a side that disagrees with the other beyond
   `tau1` loses weight exponentially (`k ≈ 18`). Fully occluded pixels fall back to the
   sharper real frame instead of a 50/50 smear.
2. **Flow overflow** (a vector pointing 300 px away is almost always wrong) → flow magnitude
   is clamped to a fraction of the source diagonal; exposed as *Artifact suppression*.
3. **Softness from blending** → optional RCAS pass after generation re-sharpens only the
   generated frames.

The neural path reduces ghosting further because the network learns where motion fields are
untrustworthy (that is literally what RIFE-style training penalizes).

## Training your own models

Everything lives in `src/neural/training/` (PyTorch, CPU or single GPU):

```
python train_upscaler.py --data D:\dataset --epochs 60 --scale 2
python train_interpolator.py --data D:\vimeo_septuplets --epochs 120
python export_onnx.py --ckpt runs\interp\best.pt --out bin\models\neural_fg.onnx
```

* **Upscaler dataset**: pairs of (low-res, high-res) stills — game screenshots downscaled
  work well; 2–10k pairs is enough for a visible NIS-class improvement.
* **Interpolator dataset**: [Vimeo-90k septuplets](http://toflow.csail.mit.edu/) (free for
  research) or any frame-sequence folders; the loader expects `seq/frame_0..6.png`.
* Models are deliberately small (≤ 4 M params) so inference costs ≈ 1–2 ms/frame at 1080p on
  a mid-range GPU via DirectML.

## Latency discipline

The neural slot is **not allowed to break the latency budget**:

* inference is enqueued on the same tick that needs the frame (no prefetch queue);
* if inference exceeds the tick budget twice in a row, the engine auto-reverts to the flow
  path for that session and logs it (visible in the HUD as `FG: flow (neural fallback)`);
* the HUD's p95 latency number is the acceptance test for any model you train — if a model
  makes p95 worse than 1.5 display ticks, it is too heavy, full stop.
