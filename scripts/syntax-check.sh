#!/usr/bin/env bash
# FlowScale — MSVC-equivalent syntax check for every Windows translation unit.
#
# Compiles each source of the FlowScale GUI target with g++ -fsyntax-only
# against the vendored stub Windows SDK (tools/win-stubs/include) that mirrors
# the real API signatures. This catches, before CI does: missing includes,
# wrong arities, renamed APIs, bad COM/WinRT usage — the whole class of errors
# that previously only surfaced on the GitHub Actions Windows runners.
#
# Runs anywhere gcc/clang + bash exist (the Linux CI job runs it on every push).
#
# Usage:  bash scripts/syntax-check.sh            (standard build surface)
#         bash scripts/syntax-check.sh --neural   (also FS_WITH_NEURAL TU; needs
#                                                  onnxruntime headers, fetched
#                                                  automatically into _deps/)
set -u
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STUBS="$REPO/tools/win-stubs/include"

SRCS=(
  src/core/Settings.cpp
  src/core/Profile.cpp
  src/core/Stats.cpp
  src/app/main.cpp
  src/app/App.cpp
  src/app/MainWindow.cpp
  src/core/Engine.cpp
  src/gfx/Device.cpp
  src/gfx/Presenter.cpp
  src/gfx/CShader.cpp
  src/gfx/Hud.cpp
  src/capture/WGCCapture.cpp
  src/capture/DDACapture.cpp
  src/scaling/ScalerChain.cpp
  src/framegen/FrameGen.cpp
  src/neural/NeuralEngine.cpp
)

WITH_NEURAL=0
[ "${1:-}" = "--neural" ] && WITH_NEURAL=1

fail=0
check() {
  local src="$1"; shift
  if ! g++ -fsyntax-only -std=c++20 -Wno-unknown-pragmas \
        -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE \
        -D_WIN32_WINNT=0x0A00 '-DFS_VERSION="0.1.0"' \
        -I"$STUBS" -I"$REPO/src" "$@" "$REPO/$src" 2>"$src.err"; then
    echo "FAIL  $src"
    grep -E "error|Error" "$src.err" | head -12
    fail=1
  else
    echo "ok    $src"
  fi
  rm -f "$src.err"
}

echo "== FlowScale Windows TU syntax check (g++ -fsyntax-only, stub SDK) =="
for s in "${SRCS[@]}"; do check "$s"; done

# Neural variant against the REAL onnxruntime headers (fetched on demand).
if [ "$WITH_NEURAL" = "1" ]; then
  echo "-- neural variant (FS_WITH_NEURAL, real onnxruntime headers) --"
  ORT_VER="1.30.1"
  ORT_DIR="$REPO/_deps/ort-headers-$ORT_VER"
  if [ ! -f "$ORT_DIR/onnxruntime_cxx_api.h" ]; then
    echo "   fetching onnxruntime-win-x64-$ORT_VER headers ..."
    mkdir -p "$REPO/_deps"
    if command -v curl >/dev/null && command -v unzip >/dev/null; then
      curl -sL --max-time 300 -o "$REPO/_deps/ort.zip" \
        "https://github.com/microsoft/onnxruntime/releases/download/v$ORT_VER/onnxruntime-win-x64-$ORT_VER.zip" \
      && unzip -q -o "$REPO/_deps/ort.zip" "onnxruntime-win-x64-$ORT_VER/include/*" -d "$REPO/_deps/" \
      && rm -rf "$ORT_DIR" && mv "$REPO/_deps/onnxruntime-win-x64-$ORT_VER/include" "$ORT_DIR" \
      && rm -f "$REPO/_deps/ort.zip"
    fi
  fi
  if [ -f "$ORT_DIR/onnxruntime_cxx_api.h" ]; then
    if ! g++ -fsyntax-only -std=c++20 -Wno-unknown-pragmas \
          -D_WIN32 -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE \
          -D_WIN32_WINNT=0x0A00 -DFS_WITH_NEURAL=1 '-DFS_VERSION="0.1.0"' \
          -I"$STUBS" -I"$ORT_DIR" -I"$REPO/src" "$REPO/src/neural/NeuralEngine.cpp" 2>/tmp/fs-neural.err; then
      echo "FAIL  src/neural/NeuralEngine.cpp (FS_WITH_NEURAL)"
      grep -E "error|Error" /tmp/fs-neural.err | head -12
      fail=1
    else
      echo "ok    src/neural/NeuralEngine.cpp (FS_WITH_NEURAL)"
    fi
  else
    echo "SKIP  src/neural/NeuralEngine.cpp (FS_WITH_NEURAL) - onnxruntime headers unavailable"
  fi
fi

exit $fail
