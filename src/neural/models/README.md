# Models directory

Drop trained ONNX models here (only used when FlowScale was built with
`-DFLOWSCALE_WITH_NEURAL=ON`):

| File | Contract | Trained by |
|---|---|---|
| `neural_upscale.onnx` | input `src` `1x3xHxW` NCHW float32 (RGB 0..1) -> output `1x3xsHxsW` | `src/neural/training/train_upscaler.py` |
| `neural_fg.onnx` | inputs `prev`/`cur` `1x3xHxW` + `t` scalar -> output `1x3xHxW` | `src/neural/training/train_interpolator.py` |

Execution: DirectML EP when `onnxruntime_providers_directml.dll` sits next to the exe,
CPU otherwise. Missing models = the flow/scaler pipelines run instead — the app never
requires a model to work.

If the HUD shows `FG: flow (neural fallback)` your model exceeded the tick budget twice
in a row; retrain smaller or lower the resolution (see docs/NEURAL_ENGINE.md).
