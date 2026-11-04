// FlowScale — LS-style control window, rendered entirely with Direct2D.
//
// The UI is NOT native widgets: the window background, sidebar, section
// panels, sliders, switches and the accent Scale button are all painted
// every frame with D2D (anti-aliased rounded rects) and DWrite text.
// Only the combos (target / algorithm / FG mode / quality) and the FPS-cap
// edit remain native, restyled dark via owner-draw + DarkMode_Explorer.
#pragma once
#include "core/Engine.h"
#include "core/Profile.h"
#include <d2d1_1.h>
#include <dwrite.h>
#include <string>
#include <vector>

namespace fs {

class MainWindow {
public:
    bool Create(HINSTANCE hInst);
    int  Run();                                     // message loop

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Handle(HWND h, UINT msg, WPARAM wp, LPARAM lp);

    // ---- geometry (physical pixels, computed by Layout) ----
    struct Geo {
        RECT scaleBtn{}, refresh{}, plus{}, minus{}, shaders{};
        RECT slScale{}, slCas{}, slRcas{}, slArt{};      // slider track rects
        RECT swHud{}, swPred{}, swHdr{};                 // switch rects
        std::vector<RECT> profileRows;                   // [0] = "Default"
    } geo_;

    // Virtual (painted) hit ids.
    enum {
        HIT_NONE = 0, HIT_SCALE, HIT_REFRESH, HIT_PLUS, HIT_MINUS, HIT_SHADERS,
        HIT_SL_SCALE, HIT_SL_CAS, HIT_SL_RCAS, HIT_SL_ART,
        HIT_SW_HUD, HIT_SW_PRED, HIT_SW_HDR,
        HIT_PROFILE = 100,                               // + row index
    };

    // ---- D2D ----
    void    D2DInit();                                   // factories + text formats
    void    RTEnsure();                                  // (re)create hwnd render target
    void    RTPaint();
    void    PaintSidebar();
    void    PaintMain();
    CPtr<ID2D1SolidColorBrush> Brush(unsigned rgb, float a = 1.0f);
    void    FillRound(const RECT& r, float rad, unsigned rgb, float a = 1.0f);
    void    Txt(const RECT& r, const wchar_t* s, IDWriteTextFormat* f, unsigned rgb, float a = 1.0f);
    void    DrawSlider(const RECT& track, float t, bool hot);
    void    DrawSwitch(const RECT& sw, bool on, bool hot);
    void    HoverGlyphButton(const RECT& r, const wchar_t* glyph, bool hot);
    void    DrawComboItem(const DRAWITEMSTRUCT* di) const;

    // ---- behavior ----
    void    BuildChildren();                             // native combos + edit
    void    MoveChild(int id, int x, int y, int w, int h);
    void    Layout();                                    // DPI-scale + compute geo_
    void    SyncComboSelections();
    void    CollectSettings(Settings& s) const;
    void    ApplySettingsLive();
    void    RefreshTargetList();
    void    RefreshProfileRows();
    void    SelectProfile(int idx);                      // 0 = Default
    void    SaveCurrentAsProfile();
    void    DeleteSelectedProfile();

    void    BeginCountdown();                            // 5..1 then StartEngine
    void    CancelCountdown(const wchar_t* why);
    void    CountTick();
    void    StartEngine();
    void    StopEngine();
    void    UpdateStatus();                              // statusText_ + running_

    int     HitTest(int x, int y) const;
    float   SliderT(const RECT& track, int x) const;     // 0..1 from x
    void    SetSliderFromX(int hit, int x);
    void    OpenShaderFolder();

    static constexpr wchar_t kClass[] = L"FlowScaleMainWnd";
    // Logical (96 dpi) design size of the client area.
    static constexpr int kClientW = 760, kClientH = 744, kSidebarW = 224;

    // ---- window / DPI ----
    HWND     hwnd_ = nullptr;
    HINSTANCE inst_ = nullptr;
    UINT     dpi_ = 96;
    int Dp(int v) const { return int(int64_t(v) * dpi_ / 96); }
    float Dpf(float v) const { return v * float(dpi_) / 96.f; }

    // ---- native controls ----
    enum { IDC_TARGET = 101, IDC_ALGO, IDC_FGMODE, IDC_QUAL, IDC_FPS_CAP };
    std::vector<TargetWindow> targets_;
    ProfileStore profiles_;
    std::vector<std::wstring> profileNames_;             // [0] = L"Default"
    int  selProfile_ = 0;
    HFONT gfont_ = nullptr;                              // Segoe UI for native ctrls
    HBRUSH gdiPanelBrush_ = nullptr;                     // dark ctl background

    // ---- D2D objects ----
    CPtr<ID2D1Factory1>        d2d_;
    CPtr<IDWriteFactory>       dwrite_;
    CPtr<ID2D1HwndRenderTarget> rt_;
    CPtr<IDWriteTextFormat> fTitle_, fSection_, fLabel_, fSmall_, fTiny_,
                            fValue_, fButton_, fCount_;
    std::vector<std::pair<unsigned, CPtr<ID2D1SolidColorBrush>>> brushCache_;

    // ---- state ----
    Settings ui_;                                        // slider/toggle/checkbox values
    int      hover_ = HIT_NONE;
    int      drag_ = 0;                                  // slider hit id being dragged
    int      countdown_ = 0;                             // seconds left; 0 = idle
    bool     running_ = false;
    bool     building_ = false;                          // suppress live-apply while syncing
    std::wstring statusText_ = L"Stopped";
    static constexpr UINT_PTR kTimerStatus = 4, kTimerCount = 5;
};

} // namespace fs
