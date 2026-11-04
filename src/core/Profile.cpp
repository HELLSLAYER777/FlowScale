#include "core/Profile.h"
#include "util/Common.h"
#include "util/Log.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace fs {

using json = nlohmann::json;

std::wstring ProfileStore::FilePath() const { return AppDataDir() + L"\\profiles.json"; }

void ProfileStore::Load() {
    profiles_.clear();
    std::ifstream f{ std::filesystem::path(FilePath()) };
    if (!f) return;

    json j = json::parse(f, nullptr, false);
    if (j.is_discarded() || !j.is_array()) return;

    for (const auto& e : j) {
        Profile p;
        if (!e.contains("name")) continue;
        p.name = Utf8ToWide(e["name"].get<std::string>());
        if (e.contains("matchExe"))   p.matchExe   = Utf8ToWide(e["matchExe"].get<std::string>());
        if (e.contains("matchTitle")) p.matchTitle = Utf8ToWide(e["matchTitle"].get<std::string>());
        if (e.contains("settings"))   p.settings.FromJson(e["settings"].dump());
        profiles_.push_back(std::move(p));
    }
    LOGI("Loaded %zu profiles", profiles_.size());
}

bool ProfileStore::Save() const {
    json j = json::array();
    for (const auto& p : profiles_) {
        j.push_back({
            { "name",      WideToUtf8(p.name) },
            { "matchExe",  WideToUtf8(p.matchExe) },
            { "matchTitle",WideToUtf8(p.matchTitle) },
            { "settings",  json::parse(p.settings.ToJson(), nullptr, false) },
        });
    }
    std::ofstream f{ std::filesystem::path(FilePath()), std::ios::trunc };
    if (!f) { LOGE("Cannot write profiles.json"); return false; }
    f << j.dump(2);
    return true;
}

void ProfileStore::Upsert(const Profile& p) {
    const auto it = std::find_if(profiles_.begin(), profiles_.end(),
        [&](const Profile& e) { return _wcsicmp(e.name.c_str(), p.name.c_str()) == 0; });
    if (it != profiles_.end()) *it = p; else profiles_.push_back(p);
}

bool ProfileStore::Erase(const std::wstring& name) {
    const auto it = std::remove_if(profiles_.begin(), profiles_.end(),
        [&](const Profile& e) { return _wcsicmp(e.name.c_str(), name.c_str()) == 0; });
    const bool removed = it != profiles_.end();
    profiles_.erase(it, profiles_.end());
    return removed;
}

static bool ContainsCI(const std::wstring& hay, const std::wstring& needle) {
    if (needle.empty()) return false;
    return StrStrIW(hay.c_str(), needle.c_str()) != nullptr;
}

std::optional<Profile> ProfileStore::Match(const std::wstring& exe, const std::wstring& title) const {
    for (const auto& p : profiles_) {
        if (!p.matchExe.empty() && !ContainsCI(exe, p.matchExe)) continue;
        if (!p.matchTitle.empty() && !ContainsCI(title, p.matchTitle)) continue;
        if (p.matchExe.empty() && p.matchTitle.empty()) continue;
        return p;
    }
    return std::nullopt;
}

} // namespace fs
