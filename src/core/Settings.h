// FlowScale — user settings model + JSON persistence.
#pragma once
#include <string>
#include "util/Common.h"

namespace fs {

enum class ScaleAlgo { Nearest, Bilinear, SharpBilinear, Lanczos3, FS1, UserShader };
enum class FgMode    { Off, X2, X3, X4 };
enum class FgQuality { Performance, Balanced, Quality };

struct Settings {
    ScaleAlgo  algo       = ScaleAlgo::Lanczos3;
    float      scaleFactor = 1.5f;        // source window -> monitor fill factor
    FgMode     fgMode     = FgMode::Off;
    FgQuality  fgQuality  = FgQuality::Balanced;
    bool       predictive = false;        // extrapolate motion to t<=1.5 (lower latency)
    float      casAmount  = 0.30f;        // pre-sharpen 0..1
    float      rcasAmount = 0.40f;        // post-sharpen on generated frames 0..1
    float      artifactSuppression = 0.50f; // 0..1: clamps flow + occlusion strength
    bool       showHud    = true;
    bool       hdrPass    = false;        // try HDR swapchain when capture is float16
    int        fpsCap     = 0;            // 0 = display refresh; else hard present cap
    std::wstring userShaderName;          // file in shaders/user (when algo == UserShader)

    // Target descriptor (what we capture).
    HWND       targetHwnd = nullptr;
    std::wstring targetTitle;
    std::wstring targetExe;

    // ---- serialization (nlohmann-based, implemented in Settings.cpp) ----
    std::string ToJson() const;
    void        FromJson(const std::string& text);

    // ---- conversions for the UI layer ----
    static const wchar_t* AlgoName(ScaleAlgo a);
    static const wchar_t* FgModeName(FgMode m);
    static const wchar_t* FgQualityName(FgQuality q);
};

} // namespace fs
