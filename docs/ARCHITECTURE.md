# Architecture

FlowScale is a display-side scaling + frame generation pipeline: it never injects into a game.
It watches a window, and renders a processed stream into its own borderless topmost window.

```
                                   (present thread owns the D3D11 immediate context)
┌──────────┐  frames   ┌───────────────┐   ┌────────────────────────────────────────────┐
│  Game /  ├──────────►│  Capture       │   │              Present thread                │
│  Video   │  (GPU)    │  WGC per-window│──►│ 1. waitable swapchain tick (display cadence)│
│  player  │           │  DDA per-screen│   │ 2. Pacer: Real or Generated(t) this tick?   │
└──────────┘           └───────────────┘   │ 3. Real → copy newest frame, update history │
                                           │ 4. ScalerChain (compute) → final texture    │
                                           │ 5. FrameGen flow passes  (Generated ticks)  │
                                           │ 6. HUD (D2D) → Present()                    │
                                           └───────────────┬────────────────────────────┘
                                                           ▼
                                              borderless flip-model swapchain
```

## Pipeline stages

### 1. Capture — `src/capture/`
Two interchangeable backends behind `ICapture`:

* **WGCCapture** — `Windows.Graphics.Capture` (per-window, hardware cursor excluded,
  free-threaded frame pool). This is the default for game windows; it is the same public API
  class that OBS uses. Frames arrive on a threadpool thread; the callback only stores the
  surface reference + QPC timestamp — no D3D calls happen on that thread.
* **DDACapture** — DXGI Desktop Duplication for full-monitor targets or pre-19041 systems.
  Polled (non-blocking, 0 ms timeout) from the present thread each tick.

Both deliver textures on the FlowScale D3D11 device, so frames never round-trip through system
memory.

### 2. Scaler chain — `src/scaling/`
Compute passes, each following one binding contract
(`t0 = source SRV, u0 = destination UAV, cb0 = { srcSize, dstSize, params }`):

`[optional CAS pre-sharpen] → [scale pass] → [optional RCAS post-sharpen]`

Shipped scale passes: `nearest`, `bilinear`, `sharp_bilinear`, `lanczos3`, `fs1`
(edge-adaptive, our own kernel). Any `.hlsl` that follows the contract can be dropped into
`bin/shaders/user/` and hot-reloaded with F5 — see [SHADERS.md](SHADERS.md).

### 3. Frame generation — `src/framegen/`
Flow-based interpolation, no ML required:

1. `rgba_to_y` — luma extraction, downsampled into a mip-like pyramid (1/4, 1/2).
2. `flow_coarse` — 8×8-block SAD search at 1/4 resolution over a ±search-radius window.
3. `flow_refine` — refine at 1/2 resolution, seeded by the upsampled coarse field;
   parabola sub-pixel fit on the 3×3 SAD neighborhood.
4. `warp_blend` — for each output pixel: sample the flow field bilinearly, warp the previous
   and current real frames to sub-frame position `t`, weight each side by occlusion confidence
   (luma disagreement, forward sample support), normalize, blend. Flow magnitude and blend
   weights are clamped ("artifact suppression").

Quality presets map to grid step / pyramid depth / search radius:

| Preset | Grid (src px) | Pyramid | Search radius | Sub-pixel |
|---|---|---|---|---|
| Performance | 16 | 1/4 | 12 | no |
| Balanced | 8 | 1/4 + 1/2 | 12 / 5 | yes |
| Quality | 8 | 1/4 + 1/2 | 16 / 6 | yes |

### 4. Neural engine (optional) — `src/neural/`
If `FS_WITH_NEURAL` is compiled in and `bin/models/*.onnx` exist:
`neural_upscale.onnx` replaces the spatial scale pass, `neural_fg.onnx` replaces (or refines)
the flow-based generator. ONNX Runtime executes on the same adapter; DirectML EP is used when
available at runtime, otherwise CPU EP. See [NEURAL_ENGINE.md](NEURAL_ENGINE.md).

### 5. Presentation — `src/gfx/Presenter.cpp`
Borderless `WS_POPUP` window covering the target monitor, flip-discard swapchain,
`Scaling = NONE`, waitable object with **max frame latency = 1**. The present thread blocks on
the waitable object each tick, which is the pacing source (optionally capped further by the
FPS limiter). HDR: when the capture texture is `R16G16B16A16_FLOAT` and the monitor reports
HDR, the swapchain is created float16 and tagged `RGB_FULL_G2084_NONE_P2020` when accepted.

## Threading model

| Thread | Owns | Never touches |
|---|---|---|
| UI thread | Win32 window, hotkeys, profiles | D3D11 |
| Capture (threadpool) | Frame surface ref + timestamp | D3D11 calls |
| Present | Immediate context, swapchain, all passes | Win32 message pump |

Communication is: atomic settings snapshot (UI → present), mutex-guarded 2-slot frame inbox
(capture → present), and `std::stop_token` for shutdown. There is exactly **one** in-flight
frame between capture and present: latency is bounded by ~1 source frame + 1 display tick.

## Latency budget (HUD-verified)

```
game renders ──► compositor ──► capture arrives ──► present thread ticks ──► scanout
                                t_arrive            ≤ 1 tick later          + queue
```

The HUD measures `present submitted − frame arrived` for the real frame currently on screen
(p50 / p95 over a 2 s window). Typical budget: 60 Hz source, 120 Hz display, x2 mode →
8–12 ms. The **predictive motion** option extrapolates up to `t = 1.5` between real frames
(lowest latency, slightly higher ghosting risk on cut scenes).

## Why per-window capture + topmost window

This is the Lossless Scaling trick, and it is public API surface: the game keeps rendering in
a (possibly minimized or occluded) window; we capture it directly and present our processed
copy fullscreen. No hooks, no driver installs, works with any renderer that produces a window.
