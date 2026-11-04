# Building FlowScale

## No toolchain? Build on GitHub instead

Push the repo to GitHub and the included Actions workflows compile, test and package it for
you on GitHub's Windows runners (see the *Compile it on GitHub* section in the README).
Artifacts of each CI run contain a ready-to-run zip; pushing a `v*` tag attaches both
variants to a GitHub Release.

## Prerequisites (Windows)

1. **Visual Studio 2022** (Community is fine) with:
   * Workload: *Desktop development with C++*
   * Individual components: **C++/WinRT build tools**, **Windows 10/11 SDK (10.0.19041 or newer)**
2. **CMake 3.24+** (bundled with VS works)
3. **vcpkg**, with the `VCPKG_ROOT` environment variable set:
   ```powershell
   git clone https://github.com/microsoft/vcpkg C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   setx VCPKG_ROOT C:\vcpkg
   ```

## Build (standard, no neural engine)

```powershell
cd FlowScale
cmake --preset windows-release        # configures, fetches nlohmann-json via vcpkg
cmake --build --preset windows-release
```

Output: `build\Release\FlowScale.exe` with `shaders\` copied next to it.

## Build with the neural engine (ONNX Runtime)

Two ways to provide ONNX Runtime — **the prebuilt route is what CI uses**:

### Route A (recommended): official prebuilt binaries

```powershell
powershell -ExecutionPolicy Bypass -File scripts\fetch-onnxruntime.ps1   # downloads + SHA-256 verifies ONNX Runtime into _deps\
cmake -B build -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
    -DFLOWSCALE_WITH_NEURAL=ON `
    -DONNXRUNTIME_ROOT_DIR="$PWD\_deps\onnxruntime-win-x64-1.30.1"
cmake --build build --config Release
```

`fetch-onnxruntime.ps1` pins version **1.30.1** (edit `-Version` to bump; add its SHA-256 to
the script's table). The runtime `onnxruntime.dll` / `onnxruntime_providers_shared.dll` are
copied next to the exe automatically.

### Route B: the vcpkg port (built from source — slow)

`vcpkg install --x-feature=neural` compiles ONNX Runtime plus its toolchain (protobuf,
abseil, onnx …) from source; expect a multi-hour build. CMake picks it up automatically via
`find_package` when `-DFLOWSCALE_WITH_NEURAL=ON` is set without `ONNXRUNTIME_ROOT_DIR`.

This enables the `bin/models/*.onnx` integration. For GPU execution install the
[DirectML execution provider](https://learn.microsoft.com/en-us/windows/ai/directml/):
drop `DirectML.dll` + `onnxruntime_providers_directml.dll` next to `FlowScale.exe`; the engine
detects them at runtime and falls back to CPU EP otherwise. Model training happens on any
machine with PyTorch — see [NEURAL_ENGINE.md](NEURAL_ENGINE.md).

## Run the pure-math tests anywhere (even Linux/macOS)

The pacing state machine, optical-flow reference math and sampler math are deliberately
dependency-free and tested by `tests/math_tests.cpp`:

```bash
cmake -B build-tests -DFLOWSCALE_BUILD_TESTS=ON && cmake --build build-tests && ctest --test-dir build-tests
```

## Syntax-check the Windows target without MSVC

`scripts/syntax-check.sh` compiles every Windows translation unit with `g++ -fsyntax-only`
against a small stub Windows/D3D/DXGI/D2D/WinRT SDK in `tools/win-stubs/` that mirrors the
real API signatures. It catches the errors that would otherwise only appear on MSVC —
missing includes, wrong argument counts, renamed APIs, bad COM/WinRT usage — on any machine
(and the Linux CI job runs it on every push). `--neural` additionally checks the
FS_WITH_NEURAL build of `NeuralEngine.cpp` against the real ONNX Runtime headers
(downloaded into `_deps/` on first use).

```bash
bash scripts/syntax-check.sh            # all Windows TUs
bash scripts/syntax-check.sh --neural   # + neural TU with real ONNX Runtime headers
```

## Packaging

```powershell
powershell -File scripts\build.ps1          # configure + build + collect a zip under dist\
```

## Troubleshooting

| Symptom | Fix |
|---|---|
| `this vcpkg instance requires a manifest with a specified baseline` | Your vcpkg clone is older than the manifest's `builtin-baseline` — update it: `git -C %VCPKG_ROOT% pull` (or re-clone and bootstrap) |
| `'winrt/Windows.Graphics.Capture.h': No such file` | Install the *C++/WinRT build tools* individual component in VS Installer, or use a Windows SDK ≥ 19041 |
| WGC reports unsupported | Windows build < 19041 — use monitor targets (DDA path) or update Windows |
| Executable blocked by SmartScreen | Unsigned dev build — *More info → Run anyway*, or sign it yourself |
| Blank presenter window | Game must be *windowed/borderless* (not exclusive fullscreen); FlowScale cannot capture exclusive-fullscreen swapchains — the game's own settings must be windowed, exactly like any capture-based scaler |
| Anti-cheat complains | Use offline/single-player titles; FlowScale injects nothing, but capture-based tools are still flagged by a few AC kernels |
