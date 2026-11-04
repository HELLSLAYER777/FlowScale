# Third-party notices

FlowScale's own code is MIT-licensed (see `LICENSE`). It builds on, or takes algorithmic
inspiration from, the following projects:

## Incorporated at build time

| Component | License | Use |
|---|---|---|
| [nlohmann/json](https://github.com/nlohmann/json) | MIT | Profile & settings serialization |
| [ONNX Runtime](https://onnxruntime.ai) *(optional, `FLOWSCALE_WITH_NEURAL=ON`)* | MIT | Neural upscaler / frame interpolator inference |
| [Microsoft DirectML](https://github.com/microsoft/DirectML) *(optional EP for ONNX Runtime)* | MIT | GPU-accelerated ONNX execution on any DX12 GPU |
| Windows SDK / C++/WinRT | Microsoft license | Windows.Graphics.Capture, D3D11, D2D |

## Algorithm references (original reimplementations, no source copied)

| Algorithm | Reference | License of reference | Where in FlowScale |
|---|---|---|---|
| RCAS sharpener | AMD FidelityFX Super Resolution 1.0 public shader docs | MIT (AMD FidelityFX-SDK samples) | `scaling/shaders/rcas.hlsl` — reimplemented from the public description |
| CAS contrast-adaptive sharpening | AMD FidelityFX CAS public docs | MIT | `scaling/shaders/cas.hlsl` — reimplemented |
| Edge-adaptive upscaling concept | AMD FidelityFX EASU (concept) / NVIDIA NIS (concept) | MIT | `scaling/shaders/fs1.hlsl` — original kernel, not a port |
| RIFE video interpolation | [Practical-RIFE](https://github.com/megvii-research/ECCV2022-RIFE) paper + code | MIT (code), model weights CC-BY-4.0 | `neural/training/train_interpolator.py` — simplified architecture, trained by you |
| Block-matching optical flow | Classical literature (SAD multi-level search) | n/a | `framegen/shaders/*.hlsl` |

## Not included, on purpose

* **Lossless Scaling** — closed source. FlowScale contains none of its code or assets.
* **NVIDIA DLSS / DLSS-NG** — proprietary, NDA-gated SDK. Cannot be embedded in any third-party
  program; FlowScale uses the open NIS-class approach and its own trainable networks instead.

If you distribute FlowScale binaries, keep this file next to the executable.
