// FlowScale — ONNX Runtime neural engine (optional, FS_WITH_NEURAL).
//
// Executes trained models for:
//   * neural upscaling     : src 1x3xHxW   -> dst 1x3x(sH)x(sW)
//   * neural interpolation : prev,cur 1x3xHxW + t -> out 1x3xHxW
// Execution provider order at runtime: DirectML (any DX12 GPU) -> CPU.
// The engine never hard-depends on a model being present; the flow/scaler
// pipelines are the always-available fallback (see docs/NEURAL_ENGINE.md).
#pragma once
#include <memory>
#include <vector>
#include "core/Settings.h"
#include "util/Common.h"

struct OrtSession;      // opaque (ORT C++ API hidden from the rest of the build)

namespace fs {

struct NeuralTensor {  // NHWC float32 CPU staging, converted to NCHW by the engine
    std::vector<float> data;
    uint32_t w = 0, h = 0;
};

class NeuralEngine {
public:
    static bool Compiled() {
#ifdef FS_WITH_NEURAL
        return true;
#else
        return false;
#endif
    }

    bool Init();
    void Shutdown();

    // Model discovery in <exe>\models : neural_upscale.onnx / neural_fg.onnx
    bool HasUpscaler() const;
    bool HasInterpolator() const;

    // Upscale: src RGBA8 (CPU staging) -> dst RGBA8 at (w*scale, h*scale).
    bool Upscale(const NeuralTensor& src, NeuralTensor& dst, float scale);

    // Interpolate: two RGBA8 frames + sub-frame position -> one RGBA8 frame.
    bool Interpolate(const NeuralTensor& prev, const NeuralTensor& cur,
                     double t, NeuralTensor& out);

    const std::wstring& ProviderName() const { return provider_; }

private:
    std::unique_ptr<OrtSession> upscale_, interp_;
    std::wstring provider_ = L"cpu";
    bool dmlAvailable_ = false;
};

} // namespace fs
