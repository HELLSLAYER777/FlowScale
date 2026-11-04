# FlowScale

[![CI](../../actions/workflows/ci.yml/badge.svg)](../../actions/workflows/ci.yml)

**Open frame generation & scaling for Windows** — a free, open-source alternative to Lossless
Scaling, built on Direct3D 11 compute shaders, with an optional **neural engine** (ONNX Runtime)
for ML upscaling and ML frame interpolation that you can train yourself.

FlowScale captures any window (or an entire monitor), upscales it to fill your screen, and can
generate intermediate frames to multiply motion smoothness — the same core idea as Lossless
Scaling, plus things LS does not give you.

---

## Feature highlights

| Feature | FlowScale | Notes |
|---|---|---|
| Frame generation (flow-based, no ML) | ✅ x2 / x3 / x4 | Coarse-to-fine block-matching optical flow, occlusion-aware bidirectional warping |
| Neural frame interpolation (trainable) | ✅ optional | Drop an ONNX model in `bin/models/`, or train one with the included PyTorch pipeline |
| Upscalers | Nearest, Bilinear, Sharp Bilinear, Lanczos-3, FS1 (edge-adaptive), RCAS/CAS sharpening | All original implementations in HLSL compute |
| **User shader hot-reload** | ✅ | Drop any `.hlsl` compute shader into the shader folder, press F5 — no restart |
| Per-game profiles | ✅ | Auto-matched by executable name or window title, stored as JSON |
| Latency HUD | ✅ | Live capture→present latency p50/p95, input/source FPS, generated FPS |
| Frame limiter + predictive motion | ✅ | Cap present rate, or extrapolate motion past the latest real frame for lower latency |
| HDR passthrough | ✅ (basic) | Float16 path when the capture and monitor support it |
| Any GPU vendor | ✅ | NVIDIA / AMD / Intel — nothing is vendor-locked, nothing is activated |

## About DLSS (please read)

You cannot legally or technically embed NVIDIA DLSS into a third-party program: the DLSS SDK is
proprietary, distributed under NDA, tied to NVIDIA's driver and to per-game integration. Any app
that "includes DLSS" outside the official path does not exist.

FlowScale gives you the **legal routes to the same class of results**:

1. **NIS-class spatial upscaling** — spatial ML-derived upscaling whose public reference (NVIDIA
   Image Scaling) is MIT-licensed; FlowScale ships its own edge-adaptive scaler (FS1) plus RCAS
   sharpening in the same spirit.
2. **A trainable neural upscaler** — `neural/training/train_upscaler.py` trains a residual
   upscaling network on *your* footage and exports ONNX; `bin/models/neural_upscale.onnx` is
   picked up automatically. This is "learning"-based upscaling you own end-to-end.
3. **A trainable neural frame interpolator** — `neural/training/train_interpolator.py` trains a
   RIFE-style interpolation network. Trained models get near-zero ghosting on their training
   distribution and slot into the same engine behind the flow-based generator.

Low latency is treated as a first-class feature: max frame latency 1, waitable swapchain pacing,
zero-copy GPU capture, and a HUD that *measures* capture→present latency so you can verify it.

---

## Requirements

* Windows 10 20H1 (build 19041) or Windows 11
* GPU with D3D11 feature level 11.0+, `DriverConcurrentWrites` recommended
* To build locally: Visual Studio 2022 (C++ workload + C++/WinRT component) + CMake + vcpkg — see
  [docs/BUILDING.md](docs/BUILDING.md)
* **No local toolchain needed** — GitHub Actions compiles it for you (below)

## Compile it on GitHub (no Windows toolchain required)

The repo ships with GitHub Actions workflows that build, test and package everything on
GitHub's own Windows runners (free for public repositories):

1. **Push the repo to GitHub** (from the `FlowScale` folder):
   ```bash
   git init && git add -A && git commit -m "FlowScale initial import"
   git branch -M main
   git remote add origin https://github.com/<YOUR-USER>/FlowScale.git
   git push -u origin main
   ```
2. **Open the Actions tab** on your repo. A *CI* run starts automatically and builds three jobs:
   * `Windows x64 (standard)` — the app, shaders and math tests
   * `Windows x64 (ONNX neural)` — same + the neural engine linked against official prebuilt
     ONNX Runtime
   * `Linux (portable math tests)` — the dependency-free pacing/optical-flow unit tests
3. **Download your exe:** open the finished run and scroll to *Artifacts* —
   `FlowScale-win64` / `FlowScale-win64-neural` each contain a runnable zip
   (`FlowScale.exe` + `shaders\` + `models\`). Unzip anywhere and run.
4. **Publish a release** when ready:
   ```bash
   git tag v0.1.0 && git push origin v0.1.0
   ```
   The *Release* workflow builds both variants and attaches
   `FlowScale-v0.1.0-win64.zip` and `FlowScale-v0.1.0-win64-neural.zip` to a GitHub Release
   with generated release notes.

The workflows build with `Ninja Multi-Config` + the runner's preinstalled vcpkg (binary-cached),
so they do not depend on which Visual Studio version the runner image ships.

## Quick start (after building)

1. Run `FlowScale.exe`. Pick a window from the drop-down.
2. Choose a scale factor and algorithm, a frame-generation mode, and press **Start**.
3. A borderless topmost window takes over the target monitor; **Ctrl+Alt+S** stops,
   **Ctrl+Alt+G** cycles frame generation, **Ctrl+Alt+H** toggles the HUD.
4. Profiles save per-game; user shaders live in `bin/shaders/user/` (F5 reloads them).

> ⚠️ Anti-cheat: FlowScale never injects into games — it only captures like OBS does. Still, some
> online anti-cheats flag screen-capture or fullscreen-topmost overlays. Use offline/single-player
> first, exactly as you would with any capture-based tool.

## Documentation

* [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — pipeline, threading model, latency budget
* [docs/BUILDING.md](docs/BUILDING.md) — build from source, step by step
* [docs/NEURAL_ENGINE.md](docs/NEURAL_ENGINE.md) — neural upscaler / interpolator, training, ONNX
* [docs/SHADERS.md](docs/SHADERS.md) — the user-shader contract (write your own scaler in 30 lines)
* [docs/ROADMAP.md](docs/ROADMAP.md) — v0.1 → v1.0 plan

## License

FlowScale code is MIT-licensed. Third-party algorithm attributions (AMD RCAS/CAS references,
ONNX Runtime, nlohmann/json) are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
FlowScale is an original implementation; it contains no code from Lossless Scaling, NVIDIA DLSS,
or AMD FidelityFX binaries.
