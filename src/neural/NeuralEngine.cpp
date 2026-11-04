#include "neural/NeuralEngine.h"
#include "util/Common.h"
#include "util/Log.h"

#ifdef FS_WITH_NEURAL

#include <onnxruntime_cxx_api.h>
#include <algorithm>
#include <cmath>

namespace fs {

namespace {

// NCHW float32 tensor bytes <-> RGBA8 staging texture.
void RgbaToNchw(const NeuralTensor& in, std::vector<float>& out) {
    out.resize(size_t(in.w) * in.h * 3);
    float* ch[3] = { out.data(),
                     out.data() + size_t(in.w) * in.h,
                     out.data() + 2 * size_t(in.w) * in.h };
    for (uint32_t y = 0; y < in.h; ++y) {
        for (uint32_t x = 0; x < in.w; ++x) {
            const uint8_t* px = reinterpret_cast<const uint8_t*>(in.data.data())
                              + (size_t(y) * in.w + x) * 4;
            const size_t idx = size_t(y) * in.w + x;
            ch[0][idx] = px[2] / 255.0f;   // BGRA input layout, RGB tensor
            ch[1][idx] = px[1] / 255.0f;
            ch[2][idx] = px[0] / 255.0f;
        }
    }
}

void NchwToRgba(const float* in, uint32_t w, uint32_t h, NeuralTensor& out) {
    out.w = w; out.h = h;
    out.data.resize(size_t(w) * h * 4);
    const size_t plane = size_t(w) * h;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint8_t* px = reinterpret_cast<uint8_t*>(out.data.data())
                        + (size_t(y) * w + x) * 4;
            const size_t idx = size_t(y) * w + x;
            px[0] = uint8_t(std::clamp(in[2 * plane + idx], 0.0f, 1.0f) * 255.0f);
            px[1] = uint8_t(std::clamp(in[1 * plane + idx], 0.0f, 1.0f) * 255.0f);
            px[2] = uint8_t(std::clamp(in[0 * plane + idx], 0.0f, 1.0f) * 255.0f);
            px[3] = 255;
        }
    }
}

} // namespace

// Keeps I/O name strings alive until Run() returns (GetInputNameAllocated hands
// out allocator-owned memory; letting the wrapper die early would dangle).
static const char* KeepName(Ort::AllocatedStringPtr&& p,
                            std::vector<Ort::AllocatedStringPtr>& owners) {
    owners.reserve(4);
    return owners.emplace_back(std::move(p)).get();
}

} // namespace fs

// Pimpl over the ORT C++ API so the rest of the build needs no ORT headers.
// Global scope on purpose: NeuralEngine.h forward-declares ::OrtSession there,
// and the unique_ptr members must refer to exactly this type.
struct OrtSession {
    Ort::Env env{ ORT_LOGGING_LEVEL_WARNING, "FlowScale" };
    std::unique_ptr<Ort::Session> session;
    std::vector<const char*> inNames, outNames;
    size_t inCount = 0, outCount = 0;
};

namespace fs {

bool NeuralEngine::Init() {
    try {
        Ort::SessionOptions opts;
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        opts.SetIntraOpNumThreads(2);

        // DirectML EP when the provider DLL ships next to the exe (any DX12 GPU).
        HMODULE dml = GetModuleHandleW(L"onnxruntime_providers_directml.dll");
        if (!dml) dml = LoadLibraryW(L"onnxruntime_providers_directml.dll");
        if (dml) {
            using AppendFn = int(__cdecl*)(OrtSessionOptions*, int);
            const auto append = reinterpret_cast<AppendFn>(
                reinterpret_cast<void*>(GetProcAddress(dml, "OrtSessionOptionsAppendExecutionProvider_DML")));
            // Ort::SessionOptions converts implicitly to OrtSessionOptions* (its
            // contained pointer) — do NOT take the wrapper's address.
            if (append && append(opts, 0) == 0)
                provider_ = L"directml";
        }
        dmlAvailable_ = provider_ == L"directml";
        upscale_ = std::make_unique<OrtSession>();
        interp_ = std::make_unique<OrtSession>();
        LOGI("NeuralEngine initialized (provider: %ls)", provider_.c_str());
        return true;
    } catch (const Ort::Exception& e) {
        LOGE("NeuralEngine init failed: %s", e.what());
        return false;
    }
}

void NeuralEngine::Shutdown() { upscale_.reset(); interp_.reset(); }

bool NeuralEngine::HasUpscaler() const {
    if (!upscale_) return false;
    const std::wstring p = ModelsDir() + L"\\neural_upscale.onnx";
    return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool NeuralEngine::HasInterpolator() const {
    if (!interp_) return false;
    const std::wstring p = ModelsDir() + L"\\neural_fg.onnx";
    return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool NeuralEngine::Upscale(const NeuralTensor& src, NeuralTensor& dst, float scale) {
    UNREFERENCED_PARAMETER(scale);   // output size comes from the model itself
    if (!HasUpscaler()) return false;
    try {
        if (!upscale_->session) {
            const std::wstring p = ModelsDir() + L"\\neural_upscale.onnx";
            Ort::SessionOptions opts;
            opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            upscale_->session = std::make_unique<Ort::Session>(upscale_->env, p.c_str(), opts);
        }
        auto& s = *upscale_->session;
        std::vector<Ort::AllocatedStringPtr> nameOwners;
        const char* inName = KeepName(s.GetInputNameAllocated(0, Ort::AllocatorWithDefaultOptions()), nameOwners);
        const char* outName = KeepName(s.GetOutputNameAllocated(0, Ort::AllocatorWithDefaultOptions()), nameOwners);

        std::vector<float> nchw;
        RgbaToNchw(src, nchw);
        const int64_t dims[4] = { 1, 3, int64_t(src.h), int64_t(src.w) };
        Ort::Value in = Ort::Value::CreateTensor<float>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
            nchw.data(), nchw.size(), dims, 4);

        auto out = s.Run(Ort::RunOptions{ nullptr }, &inName, &in, 1, &outName, 1);
        const auto& shape = out[0].GetTensorTypeAndShapeInfo().GetShape();
        if (shape.size() != 4) return false;
        const uint32_t oh = uint32_t(shape[2]), ow = uint32_t(shape[3]);
        NchwToRgba(out[0].GetTensorData<float>(), ow, oh, dst);
        return true;
    } catch (const Ort::Exception& e) {
        LOGE("Neural upscale failed: %s", e.what());
        return false;
    }
}

bool NeuralEngine::Interpolate(const NeuralTensor& prev, const NeuralTensor& cur,
                               double t, NeuralTensor& out) {
    if (!HasInterpolator() || prev.w != cur.w || prev.h != cur.h) return false;
    try {
        if (!interp_->session) {
            const std::wstring p = ModelsDir() + L"\\neural_fg.onnx";
            Ort::SessionOptions opts;
            opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            interp_->session = std::make_unique<Ort::Session>(interp_->env, p.c_str(), opts);
        }
        auto& s = *interp_->session;
        std::vector<Ort::AllocatedStringPtr> nameOwners;
        const char* inNames[3] = {
            KeepName(s.GetInputNameAllocated(0, Ort::AllocatorWithDefaultOptions()), nameOwners),
            KeepName(s.GetInputNameAllocated(1, Ort::AllocatorWithDefaultOptions()), nameOwners),
            KeepName(s.GetInputNameAllocated(2, Ort::AllocatorWithDefaultOptions()), nameOwners)
        };
        const char* outName = KeepName(s.GetOutputNameAllocated(0, Ort::AllocatorWithDefaultOptions()), nameOwners);

        std::vector<float> a, b;
        RgbaToNchw(prev, a);
        RgbaToNchw(cur, b);
        float tv = float(t);   // CreateTensor needs a mutable data pointer
        const int64_t dims[4] = { 1, 3, int64_t(prev.h), int64_t(prev.w) };
        const int64_t tdims[1] = { 1 };
        Ort::Value inA = Ort::Value::CreateTensor<float>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
            a.data(), a.size(), dims, 4);
        Ort::Value inB = Ort::Value::CreateTensor<float>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
            b.data(), b.size(), dims, 4);
        Ort::Value inT = Ort::Value::CreateTensor<float>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
            &tv, 1, tdims, 1);

        Ort::Value inputs[3] = { std::move(inA), std::move(inB), std::move(inT) };
        auto outs = s.Run(Ort::RunOptions{ nullptr }, inNames, inputs, 3, &outName, 1);
        const auto& shape = outs[0].GetTensorTypeAndShapeInfo().GetShape();
        if (shape.size() != 4) return false;
        NchwToRgba(outs[0].GetTensorData<float>(), uint32_t(shape[3]), uint32_t(shape[2]), out);
        return true;
    } catch (const Ort::Exception& e) {
        LOGE("Neural interpolate failed: %s", e.what());
        return false;
    }
}

} // namespace fs

#else  // !FS_WITH_NEURAL — compiled out; flow pipeline is the active path.

namespace fs {
bool NeuralEngine::Init() { return false; }
void NeuralEngine::Shutdown() {}
bool NeuralEngine::HasUpscaler() const { return false; }
bool NeuralEngine::HasInterpolator() const { return false; }
bool NeuralEngine::Upscale(const NeuralTensor&, NeuralTensor&, float) { return false; }
bool NeuralEngine::Interpolate(const NeuralTensor&, const NeuralTensor&, double, NeuralTensor&) { return false; }
} // namespace fs

#endif // FS_WITH_NEURAL
