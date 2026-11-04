// FlowScale — LS-style floating control window.
#pragma once
#include "core/Engine.h"
#include "core/Profile.h"

namespace fs {

class MainWindow {
public:
    bool Create(HINSTANCE hInst);
    int  Run();                                     // message loop

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Handle(HWND, UINT, WPARAM, LPARAM);

    // UI helpers
    void BuildControls();
    void RefreshTargetList();
    void RefreshProfileList();
    void SettingsFromUI(Settings& s) const;
    void SettingsToUI(const Settings& s);
    void UpdateStatus();
    void ApplySettingsLive();                       // push UI state into the engine

    static constexpr wchar_t kClass[] = L"FlowScaleMainWnd";
    HWND hwnd_ = nullptr;
    HINSTANCE inst_ = nullptr;
    std::vector<TargetWindow> targets_;
    ProfileStore profiles_;
    std::wstring statusText_ = L"Stopped";
    UINT_PTR refreshTimer_ = 0;
    bool building_ = false;                         // suppress live-apply while loading
};

} // namespace fs
