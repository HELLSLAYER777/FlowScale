# Roadmap

## v0.1 — this release (flow core)

- [x] WGC per-window capture + DDA monitor fallback, zero-copy GPU path
- [x] Scaler chain: Nearest / Bilinear / Sharp Bilinear / Lanczos-3 / FS1 + CAS + RCAS
- [x] Flow-based frame generation x2/x3/x4 (block matching, occlusion-aware warp-blend)
- [x] Borderless flip-model presenter, max frame latency 1, waitable pacing, HDR path
- [x] LS-style UI, per-exe profiles, global hotkeys, latency/FPS HUD
- [x] User-shader hot-reload (F5)
- [x] Pure-math test suite (pacing, flow reference, samplers) — runs on any OS

## v0.2 — neural GA

- [ ] Neural engine default-on once reference models are trained and shipped
- [ ] Temporal (multi-frame) neural upscaler experiment
- [ ] Per-title neural model selection (pixel-art vs 3D vs video)
- [ ] Performance-adaptive flow quality (auto-drop preset when frame time exceeds budget)

## v0.3 — UX

- [ ] In-game HUD config, screenshot bench (before/after PNG export)
- [ ] Profile auto-learning (settings suggestions from observed content type)
- [ ] VRR/tearing-aware pacing option (ALLOW_TEARING + present fence)
- [ ] Multi-monitor presenter polish (move with target window across monitors)

## v1.0 — distribution

- [ ] Code-signing, installer, crash reporter
- [ ] Localization (EN first)
- [ ] CI: build + math tests on GitHub Actions (windows-latest), artifact upload
