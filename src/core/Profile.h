// FlowScale — per-game profiles (%APPDATA%\FlowScale\profiles.json).
//
// A profile snapshots a Settings struct plus a match rule (executable name and/or
// window title substring). When the user picks a target window, the store returns
// the first matching profile so settings snap back automatically per game.
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "core/Settings.h"

namespace fs {

struct Profile {
    std::wstring name;          // display name
    std::wstring matchExe;      // e.g. "eldo.exe" (case-insensitive, substring)
    std::wstring matchTitle;    // title substring, case-insensitive
    Settings     settings;      // the stored snapshot (target fields ignored)
};

class ProfileStore {
public:
    // Loads %APPDATA%\FlowScale\profiles.json (missing file = empty store).
    void Load();
    bool Save() const;

    const std::vector<Profile>& All() const { return profiles_; }

    void Upsert(const Profile& p);                 // matched by name
    bool Erase(const std::wstring& name);

    // First profile whose rule matches (exe, then title). Empty rules never match.
    std::optional<Profile> Match(const std::wstring& exe, const std::wstring& title) const;

private:
    std::vector<Profile> profiles_;
    std::wstring FilePath() const;
};

} // namespace fs
