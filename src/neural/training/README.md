# Training the neural models

Everything here is plain PyTorch (a single consumer GPU with 8 GB VRAM is plenty).

```bash
pip install torch torchvision pillow onnx onnxruntime
```

## 1. Neural upscaler (`neural_upscale.onnx`)

1. Build a dataset of paired stills:
   ```
   dataset/
     low/   # downscale your game captures to 50/33/25 %
     high/  # the originals
   ```
   2–10k pairs is a good start; keep the *same game* for the biggest wins.
2. Train:
   ```bash
   python train_upscaler.py --data D:\dataset --scale 2 --epochs 60
   ```
3. Export + install:
   ```bash
   python export_onnx.py --ckpt runs/upscaler/best.pt --kind upscale --out <exe dir>\models\neural_upscale.onnx
   ```

## 2. Neural frame interpolator (`neural_fg.onnx`)

1. Get sequence data — Vimeo-90k septuplets (free for research) or any folders of
   7-frame clips (`frame_0.png`..`frame_6.png`).
2. Train (RIFE-lite, ~4 M params ≈ 1–2 ms/frame via DirectML at 1080p):
   ```bash
   python train_interpolator.py --data D:\vimeo_septuplets\sequences --epochs 120
   ```
3. Export + install:
   ```bash
   python export_onnx.py --ckpt runs/interp/best.pt --kind interp --out <exe dir>\models\neural_fg.onnx
   ```

## 3. Acceptance test

Run FlowScale with the HUD on:

* status shows `FG: neural` when the model is used;
* p95 latency must stay under 1.5 display ticks — if it degrades, the engine
  auto-falls back to flow mode and flags the HUD (`FG: flow (neural fallback)`);
* ghosting check: pans over text (UI, HUD text in games) — the neural path
  should keep glyphs sharper than the flow blend at equal settings.

## Licenses for datasets

* Vimeo-90k: free for research use — check the terms on the ToFlow project page
  before any commercial redistribution of trained weights.
* Your own captures: yours, no restrictions.
