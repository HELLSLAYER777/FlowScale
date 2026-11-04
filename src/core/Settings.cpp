#include "core/Settings.h"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace fs {

using json = nlohmann::json;

static const wchar_t* kAlgoNames[]  = { L"Nearest", L"Bilinear", L"Sharp Bilinear", L"Lanczos 3", L"FS1 (Edge-adaptive)", L"User shader" };
static const wchar_t* kFgNames[]    = { L"Off", L"x2", L"x3", L"x4" };
static const wchar_t* kQualNames[]  = { L"Performance", L"Balanced", L"Quality" };

std::string Settings::ToJson() const {
    json j;
    j["algo"]         = static_cast<int>(algo);
    j["scale"]        = scaleFactor;
    j["fgMode"]       = static_cast<int>(fgMode);
    j["fgQuality"]    = static_cast<int>(fgQuality);
    j["predictive"]   = predictive;
    j["cas"]          = casAmount;
    j["rcas"]         = rcasAmount;
    j["artifactSup"]  = artifactSuppression;
    j["hud"]          = showHud;
    j["hdr"]          = hdrPass;
    j["fpsCap"]       = fpsCap;
    j["userShader"]   = WideToUtf8(userShaderName);
    j["title"]        = WideToUtf8(targetTitle);
    j["exe"]          = WideToUtf8(targetExe);
    return j.dump(2);
}

void Settings::FromJson(const std::string& text) {
    json j = json::parse(text, /*cb*/ nullptr, /*allow_exceptions*/ false);
    if (j.is_discarded() || !j.is_object()) return;
    auto get = [&](const char* k) { return j.contains(k) ? j.at(k) : json(); };

    if (auto v = get("algo");    v.is_number_integer()) algo       = static_cast<ScaleAlgo>(std::clamp(v.get<int>(), 0, 5));
    if (auto v = get("scale");   v.is_number())         scaleFactor = std::clamp(v.get<float>(), 0.25f, 6.0f);
    if (auto v = get("fgMode");  v.is_number_integer()) fgMode     = static_cast<FgMode>(std::clamp(v.get<int>(), 0, 3));
    if (auto v = get("fgQuality"); v.is_number_integer()) fgQuality = static_cast<FgQuality>(std::clamp(v.get<int>(), 0, 2));
    if (auto v = get("predictive"); v.is_boolean())     predictive = v.get<bool>();
    if (auto v = get("cas");     v.is_number())         casAmount  = std::clamp(v.get<float>(), 0.0f, 1.0f);
    if (auto v = get("rcas");    v.is_number())         rcasAmount = std::clamp(v.get<float>(), 0.0f, 1.0f);
    if (auto v = get("artifactSup"); v.is_number())     artifactSuppression = std::clamp(v.get<float>(), 0.0f, 1.0f);
    if (auto v = get("hud");     v.is_boolean())        showHud    = v.get<bool>();
    if (auto v = get("hdr");     v.is_boolean())        hdrPass    = v.get<bool>();
    if (auto v = get("fpsCap");  v.is_number_integer()) fpsCap     = std::clamp(v.get<int>(), 0, 1000);
    if (auto v = get("userShader"); v.is_string())      userShaderName = Utf8ToWide(v.get<std::string>());
    if (auto v = get("title");   v.is_string())         targetTitle = Utf8ToWide(v.get<std::string>());
    if (auto v = get("exe");     v.is_string())         targetExe   = Utf8ToWide(v.get<std::string>());
}

const wchar_t* Settings::AlgoName(ScaleAlgo a)      { return kAlgoNames[static_cast<int>(a)]; }
const wchar_t* Settings::FgModeName(FgMode m)       { return kFgNames[static_cast<int>(m)]; }
const wchar_t* Settings::FgQualityName(FgQuality q) { return kQualNames[static_cast<int>(q)]; }

} // namespace fs
