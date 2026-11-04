// FlowScale — LS-style control window, fully D2D-rendered (see MainWindow.h).
//
// Painting model: WM_PAINT -> a D2D hwnd render target redraws the whole
// window (background, sidebar, panels, sliders, switches, accent button).
// Native widgets survive only where the OS is genuinely better: the four
// combo boxes (keyboard nav + long lists) and the FPS-cap edit — both
// restyled dark via owner-draw + the DarkMode_Explorer theme class.
#include "app/MainWindow.h"
#include "util/Common.h"
#include "util/Log.h"
#include <commctrl.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <algorithm>

#pragma comment(lib, "uxtheme.lib")

namespace fs {

// ------------------------------------------------------------------ palette --
namespace pal {
    constexpr unsigned Bg      = 0x1B1C21;   // window background
    constexpr unsigned Sidebar = 0x212228;
    constexpr unsigned Panel   = 0x24252B;
    constexpr unsigned Hover   = 0x2D2E35;
    constexpr unsigned PanelIn = 0x2A2B31;   // inset controls (refresh button)
    constexpr unsigned Border  = 0x2F3038;
    constexpr unsigned Accent  = 0x3B82F6;
    constexpr unsigned AccentH = 0x4C8DFF;
    constexpr unsigned Stop    = 0xE5484D;
    constexpr unsigned StopH   = 0xF06065;
    constexpr unsigned Text    = 0xEDEDF0;
    constexpr unsigned Text2   = 0x9A9BA3;
    constexpr unsigned Text3   = 0x6B6C75;
    constexpr unsigned Sel     = 0x28394E;   // selected profile row
    constexpr unsigned Track   = 0x3A3B42;   // slider track / switch off
    constexpr unsigned ComboTx = 0xE4E4E8;
}

// Layout metrics (logical 96-dpi pixels).
namespace lay {
    constexpr int Pad        = 26;   // main column side padding
    constexpr int PanelRad   = 10;
    constexpr int PanelPad   = 16;
    constexpr int FirstPanelY = 84;
    constexpr int TargetH  = 84;
    constexpr int ScaleH   = 196;
    constexpr int FgH      = 226;
    constexpr int OverlayH = 110;
    constexpr int Gap      = 10;
}

static bool InR(const RECT& r, int x, int y) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}
static D2D1_RECT_F RF(const RECT& r) {
    return { float(r.left), float(r.top), float(r.right), float(r.bottom) };
}
static float MapRange(float v, float lo, float hi) { return (v - lo) / (hi - lo); }

// ---------------------------------------------------------------- lifecycle --
bool MainWindow::Create(HINSTANCE hInst) {
    inst_ = hInst;

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;                    // everything is D2D-painted
    wc.lpszClassName = kClass;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    profiles_.Load();
    ui_ = Settings{};

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r{ 0, 0, kClientW, kClientH };            // 96-dpi size; re-scaled on WM_CREATE
    AdjustWindowRectEx(&r, style, FALSE, WS_EX_APPWINDOW);
    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kClass, L"FlowScale", style,
                            CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                            nullptr, nullptr, hInst, this);
    if (!hwnd_) return false;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

int MainWindow::Run() {
    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return static_cast<int>(m.wParam);
}

LRESULT CALLBACK MainWindow::WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        self = static_cast<MainWindow*>(reinterpret_cast<LPCREATESTRUCTW>(lp)->lpCreateParams);
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    }
    const LRESULT r = self ? self->Handle(h, msg, wp, lp) : DefWindowProcW(h, msg, wp, lp);
    return msg == WM_NCCREATE && !self ? DefWindowProcW(h, msg, wp, lp) : r;
}

LRESULT MainWindow::Handle(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        // WM_CREATE runs inside CreateWindowExW — store the hwnd now (children
        // below need a valid parent), then re-scale the window to the real DPI.
        hwnd_ = h;
        dpi_ = GetDpiForWindow(h);
        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT r{ 0, 0, Dp(kClientW), Dp(kClientH) };
        AdjustWindowRectEx(&r, style, FALSE, WS_EX_APPWINDOW);
        SetWindowPos(h, nullptr, 0, 0, r.right - r.left, r.bottom - r.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        D2DInit();
        BuildChildren();
        RefreshProfileRows();
        RefreshTargetList();
        SyncComboSelections();
        // Global hotkeys (work while any app has focus):
        RegisterHotKey(h, 1, MOD_CONTROL | MOD_ALT, 'S');   // start / stop
        RegisterHotKey(h, 2, MOD_CONTROL | MOD_ALT, 'G');   // cycle frame-gen mode
        RegisterHotKey(h, 3, MOD_CONTROL | MOD_ALT, 'H');   // toggle FPS overlay
        SetTimer(h, kTimerStatus, 1000, nullptr);
        UpdateStatus();
        return 0;
    }

    case WM_PAINT:
        RTEnsure();
        RTPaint();
        ValidateRect(hwnd_, nullptr);
        return 0;

    case WM_ERASEBKGND:
        return 1;                                          // D2D covers every pixel

    case WM_TIMER:
        if (wp == kTimerCount) CountTick();
        else if (wp == kTimerStatus) UpdateStatus();
        return 0;

    case WM_HOTKEY: {
        if (wp == 1) {                                     // start / stop (with countdown)
            if (countdown_) CancelCountdown(L"Start cancelled");
            else if (running_) StopEngine();
            else BeginCountdown();
        } else if (wp == 2) {                              // cycle frame-gen mode
            ui_.fgMode = FgMode((int(ui_.fgMode) + 1) % 4);
            building_ = true;
            SyncComboSelections();
            building_ = false;
            ApplySettingsLive();
        } else if (wp == 3) {                              // toggle FPS overlay
            ui_.showHud = !ui_.showHud;
            ApplySettingsLive();
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }

    case WM_COMMAND: {
        const int id = LOWORD(wp), code = HIWORD(wp);
        if (code == CBN_SELCHANGE && (id == IDC_ALGO || id == IDC_FGMODE || id == IDC_QUAL ||
                                      id == IDC_TARGET)) {
            ApplySettingsLive();
        } else if (id == IDC_FPS_CAP && code == EN_CHANGE) {
            ApplySettingsLive();
        }
        return 0;
    }

    case WM_MEASUREITEM:
        reinterpret_cast<MEASUREITEMSTRUCT*>(lp)->itemHeight = UINT(Dp(24));
        return TRUE;

    case WM_DRAWITEM:
        DrawComboItem(reinterpret_cast<const DRAWITEMSTRUCT*>(lp));
        return TRUE;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, pal::ComboTx);
        SetBkColor(dc, pal::Panel);
        return reinterpret_cast<LRESULT>(gdiPanelBrush_);
    }

    case WM_MOUSEMOVE: {
        const int x = short(LOWORD(lp)), y = short(HIWORD(lp));
        if (drag_) { SetSliderFromX(drag_, x); return 0; }
        const int h = HitTest(x, y);
        if (h != hover_) { hover_ = h; InvalidateRect(hwnd_, nullptr, FALSE); }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        const int x = short(LOWORD(lp)), y = short(HIWORD(lp));
        const int hit = HitTest(x, y);
        if (hit >= HIT_SL_SCALE && hit <= HIT_SL_ART) {
            drag_ = hit;
            SetCapture(hwnd_);
            SetSliderFromX(hit, x);
        } else if (hit == HIT_SCALE) {
            if (countdown_) CancelCountdown(L"Start cancelled");
            else if (running_) StopEngine();
            else BeginCountdown();
        } else if (hit == HIT_REFRESH) {
            RefreshTargetList();
        } else if (hit == HIT_PLUS) {
            SaveCurrentAsProfile();
        } else if (hit == HIT_MINUS) {
            DeleteSelectedProfile();
        } else if (hit == HIT_SHADERS) {
            OpenShaderFolder();
        } else if (hit == HIT_SW_HUD) {
            ui_.showHud = !ui_.showHud;
            ApplySettingsLive();
        } else if (hit == HIT_SW_PRED) {
            ui_.predictive = !ui_.predictive;
            ApplySettingsLive();
        } else if (hit == HIT_SW_HDR) {
            ui_.hdrPass = !ui_.hdrPass;
            ApplySettingsLive();
        } else if (hit >= HIT_PROFILE) {
            SelectProfile(hit - HIT_PROFILE);
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        if (drag_) { drag_ = 0; ReleaseCapture(); }
        return 0;

    case WM_CAPTURECHANGED:
        drag_ = 0;
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd_, &pt);
            SetCursor(LoadCursorW(nullptr, HitTest(pt.x, pt.y) != HIT_NONE ? IDC_HAND : IDC_ARROW));
            return TRUE;
        }
        break;

    case WM_DESTROY:
        KillTimer(h, kTimerStatus);
        KillTimer(h, kTimerCount);
        UnregisterHotKey(h, 1); UnregisterHotKey(h, 2); UnregisterHotKey(h, 3);
        Engine::I().Stop();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

// --------------------------------------------------------------- D2D setup --
void MainWindow::D2DInit() {
    ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d_.GetAddressOf()),
                  "D2D1CreateFactory");
    ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                      reinterpret_cast<IUnknown**>(dwrite_.GetAddressOf())),
                  "DWriteCreateFactory");

    auto mk = [&](float px, DWRITE_FONT_WEIGHT w, DWRITE_TEXT_ALIGNMENT ta,
                  DWRITE_PARAGRAPH_ALIGNMENT pa) {
        CPtr<IDWriteTextFormat> f;
        ThrowIfFailed(dwrite_->CreateTextFormat(L"Segoe UI", nullptr, w, DWRITE_FONT_STYLE_NORMAL,
                                                DWRITE_FONT_STRETCH_NORMAL, Dpf(px), L"en-us",
                                                f.GetAddressOf()), "CreateTextFormat");
        f->SetTextAlignment(ta);
        f->SetParagraphAlignment(pa);
        return f;
    };
    fTitle_   = mk(17.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                   DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fSection_ = mk(12.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                   DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fLabel_   = mk(12.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                   DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fSmall_   = mk(12.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                   DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fTiny_    = mk(11.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                   DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fValue_   = mk(12.f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING,
                   DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    fButton_  = mk(14.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER,
                   DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    fCount_   = mk(22.f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER,
                   DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // GDI side: font + dark background brush for the native combo/edit.
    gfont_ = CreateFontW(-int(Dp(13)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    gdiPanelBrush_ = CreateSolidBrush(pal::Panel);
}

void MainWindow::RTEnsure() {
    if (rt_) return;
    RECT crc{};
    GetClientRect(hwnd_, &crc);
    const D2D1_SIZE_U size{ UINT(crc.right - crc.left), UINT(crc.bottom - crc.top) };
    ThrowIfFailed(d2d_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                               D2D1::HwndRenderTargetProperties(hwnd_, size),
                                               rt_.GetAddressOf()),
                  "CreateHwndRenderTarget");
    brushCache_.clear();
}

CPtr<ID2D1SolidColorBrush> MainWindow::Brush(unsigned rgb, float a) {
    const unsigned key = rgb | (unsigned(a * 255.f) << 24);
    for (auto& kv : brushCache_) if (kv.first == key) return kv.second;
    CPtr<ID2D1SolidColorBrush> b;
    ThrowIfFailed(rt_->CreateSolidColorBrush(D2D1::ColorF(rgb, a), b.GetAddressOf()),
                  "CreateSolidColorBrush");
    brushCache_.push_back({ key, b });
    return b;
}

void MainWindow::FillRound(const RECT& r, float rad, unsigned rgb, float a) {
    rt_->FillRoundedRectangle({ RF(r), rad, rad }, Brush(rgb, a).Get());
}

void MainWindow::Txt(const RECT& r, const wchar_t* s, IDWriteTextFormat* f, unsigned rgb, float a) {
    rt_->DrawTextW(s, UINT32(wcslen(s)), f, RF(r), Brush(rgb, a).Get());
}

void MainWindow::DrawSlider(const RECT& tr, float t, bool hot) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float cy = (tr.top + tr.bottom) / 2.f;
    const RECT line{ tr.left, int(cy - Dpf(2)), tr.right, int(cy + Dpf(2)) };
    FillRound(line, Dpf(2), pal::Track);
    const int fx = tr.left + int(t * float(tr.right - tr.left));
    if (fx > line.left + 2)
        FillRound({ tr.left, line.top, fx, line.bottom }, Dpf(2), hot ? pal::AccentH : pal::Accent);
    const float r = hot ? Dpf(7.5f) : Dpf(6.5f);
    rt_->FillEllipse({ { float(fx), cy }, r, r }, Brush(pal::Text));
}

void MainWindow::DrawSwitch(const RECT& sw, bool on, bool hot) {
    FillRound(sw, Dpf(10), on ? pal::Accent : (hot ? 0x46474F : pal::Track));
    const float cy = (sw.top + sw.bottom) / 2.f;
    const float cx = on ? sw.right - Dpf(10) : sw.left + Dpf(10);
    rt_->FillEllipse({ { cx, cy }, Dpf(6.5f), Dpf(6.5f) }, Brush(pal::Text));
}

void MainWindow::HoverGlyphButton(const RECT& r, const wchar_t* glyph, bool hot) {
    if (hot) FillRound(r, Dpf(6), pal::Hover);
    Txt(r, glyph, fLabel_, hot ? pal::Text : pal::Text2);
}

void MainWindow::DrawComboItem(const DRAWITEMSTRUCT* di) const {
    static HBRUSH bPanel = CreateSolidBrush(pal::Panel);
    static HBRUSH bAcc   = CreateSolidBrush(pal::Accent);
    const bool picked = (di->itemState & ODS_SELECTED) && !(di->itemState & ODS_COMBOBOXEDIT);
    FillRect(di->hDC, &di->rcItem, picked ? bAcc : bPanel);
    if (di->itemID == UINT(-1)) return;
    wchar_t buf[256]{};
    SendMessageW(di->hwndItem, CB_GETLBTEXT, di->itemID, reinterpret_cast<LPARAM>(buf));
    SetBkMode(di->hDC, TRANSPARENT);
    SetTextColor(di->hDC, picked ? 0xFFFFFF : pal::ComboTx);
    RECT rc = di->rcItem;
    rc.left += Dp(10);
    ::DrawTextW(di->hDC, buf, -1, &rc, UINT(DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS));
}

// ------------------------------------------------------------------ layout --
void MainWindow::MoveChild(int id, int x, int y, int w, int h) {
    HWND c = GetDlgItem(hwnd_, id);
    if (c) SetWindowPos(c, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

void MainWindow::Layout() {
    RECT crc{};
    GetClientRect(hwnd_, &crc);
    const int mx = Dp(kSidebarW) + Dp(lay::Pad);
    const int mw = crc.right - mx - Dp(lay::Pad);
    const int sw = Dp(38);                               // switch width

    geo_.scaleBtn = { mx + mw - Dp(132), Dp(24), mx + mw, Dp(64) };

    int y = Dp(lay::FirstPanelY);
    {   // TARGET
        const int p = y;
        geo_.refresh = { mx + mw - Dp(lay::PanelPad) - Dp(58), p + Dp(38),
                         mx + mw - Dp(lay::PanelPad),          p + Dp(38) + Dp(26) };
        MoveChild(IDC_TARGET, mx + Dp(lay::PanelPad), p + Dp(38),
                  mw - 2 * Dp(lay::PanelPad) - Dp(58) - Dp(10), Dp(26));
        y += Dp(lay::TargetH + lay::Gap);
    }
    {   // SCALING
        const int p = y;
        geo_.slScale = { mx + Dp(lay::PanelPad), p + Dp(104), mx + mw - Dp(lay::PanelPad), p + Dp(124) };
        geo_.slCas   = { mx + Dp(lay::PanelPad), p + Dp(152), mx + mw - Dp(lay::PanelPad), p + Dp(172) };
        MoveChild(IDC_ALGO, mx + Dp(lay::PanelPad), p + Dp(48), mw - 2 * Dp(lay::PanelPad), Dp(26));
        y += Dp(lay::ScaleH + lay::Gap);
    }
    {   // FRAME GENERATION
        const int p = y;
        const int half = (mw - 2 * Dp(lay::PanelPad) - Dp(12)) / 2;
        MoveChild(IDC_FGMODE, mx + Dp(lay::PanelPad), p + Dp(48), half, Dp(26));
        MoveChild(IDC_QUAL, mx + Dp(lay::PanelPad) + half + Dp(12), p + Dp(48), half, Dp(26));
        geo_.slRcas = { mx + Dp(lay::PanelPad), p + Dp(104), mx + mw - Dp(lay::PanelPad), p + Dp(124) };
        geo_.slArt  = { mx + Dp(lay::PanelPad), p + Dp(152), mx + mw - Dp(lay::PanelPad), p + Dp(172) };
        geo_.swPred = { mx + Dp(lay::PanelPad) + half - sw, p + Dp(184),
                        mx + Dp(lay::PanelPad) + half,      p + Dp(184) + Dp(20) };
        geo_.swHdr  = { mx + mw - Dp(lay::PanelPad) - sw, p + Dp(184),
                        mx + mw - Dp(lay::PanelPad),      p + Dp(184) + Dp(20) };
        y += Dp(lay::FgH + lay::Gap);
    }
    {   // OVERLAY
        const int p = y;
        geo_.swHud = { mx + mw - Dp(lay::PanelPad) - sw, p + Dp(36),
                       mx + mw - Dp(lay::PanelPad),      p + Dp(36) + Dp(20) };
        MoveChild(IDC_FPS_CAP, mx + mw - Dp(lay::PanelPad) - Dp(70), p + Dp(68), Dp(70), Dp(22));
        y += Dp(lay::OverlayH + lay::Gap);
    }

    // sidebar profile rows
    geo_.profileRows.assign(profileNames_.size(), RECT{});
    int py = Dp(120);
    for (auto& r : geo_.profileRows) {
        r = { Dp(20), py, Dp(kSidebarW) - Dp(20), py + Dp(34) };
        py += Dp(38);
    }
    // sidebar header buttons + bottom
    geo_.plus  = { Dp(kSidebarW) - Dp(20) - Dp(24), Dp(84), Dp(kSidebarW) - Dp(20), Dp(84) + Dp(24) };
    geo_.minus = { geo_.plus.left - Dp(30),          Dp(84), geo_.plus.left - Dp(6),  Dp(84) + Dp(24) };
    geo_.shaders = { Dp(20), crc.bottom - Dp(64), Dp(kSidebarW) - Dp(20), crc.bottom - Dp(34) };
}

void MainWindow::BuildChildren() {
    auto mk = [&](const wchar_t* cls, DWORD extra, int id) {
        HWND c = CreateWindowExW(0, cls, nullptr, WS_CHILD | WS_VISIBLE | extra,
                                 0, 0, 10, 10, hwnd_,
                                 reinterpret_cast<HMENU>(static_cast<intptr_t>(id)), inst_, nullptr);
        if (c) {
            SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(gfont_), TRUE);
            SetWindowTheme(c, L"DarkMode_Explorer", nullptr);
        }
    };
    const DWORD combo = DWORD(CBS_DROPDOWNLIST) | DWORD(CBS_OWNERDRAWFIXED) | DWORD(CBS_HASSTRINGS);
    mk(L"COMBOBOX", combo | DWORD(WS_VSCROLL), IDC_TARGET);
    mk(L"COMBOBOX", combo, IDC_ALGO);
    mk(L"COMBOBOX", combo, IDC_FGMODE);
    mk(L"COMBOBOX", combo, IDC_QUAL);
    mk(L"EDIT", DWORD(WS_BORDER) | DWORD(ES_CENTER) | DWORD(ES_NUMBER), IDC_FPS_CAP);
    SetDlgItemTextW(hwnd_, IDC_FPS_CAP, L"0");

    for (int a = 0; a <= 5; ++a)
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_ALGO), Settings::AlgoName(ScaleAlgo(a)));
    for (int m = 0; m <= 3; ++m)
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_FGMODE), Settings::FgModeName(FgMode(m)));
    for (int q = 0; q <= 2; ++q)
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_QUAL), Settings::FgQualityName(FgQuality(q)));
}

// ---------------------------------------------------------------- painting --
void MainWindow::RTPaint() {
    rt_->BeginDraw();
    D2D1_COLOR_F bg = D2D1::ColorF(pal::Bg);
    rt_->Clear(&bg);
    PaintSidebar();
    PaintMain();
    if (FAILED(rt_->EndDraw())) rt_.Reset();             // device lost: recreate next frame
}

void MainWindow::PaintSidebar() {
    RECT crc{};
    GetClientRect(hwnd_, &crc);
    const int W = Dp(kSidebarW);
    FillRound({ 0, 0, W, crc.bottom }, 0, pal::Sidebar);
    rt_->DrawLine({ float(W) + .5f, 0 }, { float(W) + .5f, float(crc.bottom) }, Brush(pal::Border), 1.f);

    // logo block
    const RECT logo{ Dp(24), Dp(20), Dp(24) + Dp(28), Dp(20) + Dp(28) };
    FillRound(logo, Dpf(8), pal::Accent);
    Txt(logo, L"F", fButton_, pal::Text);
    Txt({ Dp(24) + Dp(38), Dp(18), W - Dp(12), Dp(42) }, L"FlowScale", fTitle_, pal::Text);
    Txt({ Dp(24) + Dp(38), Dp(44), W - Dp(12), Dp(60) }, L"v" FS_VERSION, fTiny_, pal::Text3);

    // profiles caption + add/delete
    Txt({ Dp(24), Dp(88), geo_.minus.left, Dp(106) }, L"PROFILES", fTiny_, pal::Text3);
    HoverGlyphButton(geo_.plus, L"+", hover_ == HIT_PLUS);
    HoverGlyphButton(geo_.minus, L"\x2013", hover_ == HIT_MINUS);

    // profile rows
    for (size_t i = 0; i < geo_.profileRows.size(); ++i) {
        const RECT& r = geo_.profileRows[i];
        if (r.bottom > crc.bottom - Dp(168)) break;      // keep legend visible
        const bool sel = int(i) == selProfile_;
        if (sel)                         FillRound(r, Dpf(8), pal::Sel);
        else if (hover_ == HIT_PROFILE + int(i)) FillRound(r, Dpf(8), pal::Hover);
        Txt({ r.left + Dp(12), r.top, r.right - Dp(8), r.bottom },
            profileNames_[i].c_str(), fLabel_, sel ? pal::Text : pal::Text2);
    }

    // hotkeys legend
    const int by = crc.bottom - Dp(150);
    rt_->DrawLine({ float(Dp(20)), float(by - Dp(12)) },
                  { float(W - Dp(20)), float(by - Dp(12)) }, Brush(pal::Border), 1.f);
    static const wchar_t* keys[] = {
        L"Ctrl+Alt+S   Start / Stop",
        L"Ctrl+Alt+G   Cycle frame gen",
        L"Ctrl+Alt+H   Toggle overlay",
    };
    for (int i = 0; i < 3; ++i)
        Txt({ Dp(24), by + i * Dp(18), W - Dp(12), by + i * Dp(18) + Dp(16) }, keys[i], fTiny_, pal::Text3);

    // shader folder row
    if (hover_ == HIT_SHADERS) FillRound(geo_.shaders, Dpf(8), pal::Hover);
    Txt({ geo_.shaders.left + Dp(12), geo_.shaders.top, geo_.shaders.right, geo_.shaders.bottom },
        L"Open shader folder", fLabel_, pal::Text2);
}

void MainWindow::PaintMain() {
    RECT crc{};
    GetClientRect(hwnd_, &crc);
    const int mx = Dp(kSidebarW) + Dp(lay::Pad);
    const int mw = crc.right - mx - Dp(lay::Pad);

    // header: profile name + status
    const int ti = std::clamp(selProfile_, 0, int(profileNames_.size()) - 1);
    Txt({ mx, Dp(22), mx + mw - Dp(160), Dp(46) }, profileNames_[ti].c_str(), fTitle_, pal::Text);
    Txt({ mx, Dp(48), mx + mw - Dp(160), Dp(68) }, statusText_.c_str(), fSmall_, pal::Text2);

    // accent SCALE / countdown / STOP button
    const bool hot = hover_ == HIT_SCALE;
    const unsigned bg = (running_ || countdown_) ? (hot ? pal::StopH : pal::Stop)
                                                 : (hot ? pal::AccentH : pal::Accent);
    FillRound(geo_.scaleBtn, Dpf(8), bg);
    if (countdown_ > 0) {
        Txt(geo_.scaleBtn, std::to_wstring(countdown_).c_str(), fCount_, pal::Text);
    } else if (running_) {
        Txt(geo_.scaleBtn, L"STOP", fButton_, pal::Text);
    } else {
        Txt(geo_.scaleBtn, L"SCALE", fButton_, pal::Text);
    }

    auto panel = [&](int y, int h, const wchar_t* title) {
        const RECT r{ mx, y, mx + mw, y + h };
        FillRound(r, Dpf(lay::PanelRad), pal::Panel);
        rt_->DrawRoundedRectangle({ RF(r), Dpf(lay::PanelRad), Dpf(lay::PanelRad) },
                                  Brush(pal::Border), 1.f);
        Txt({ mx + Dp(lay::PanelPad), y + Dp(12), mx + mw - Dp(lay::PanelPad), y + Dp(30) },
            title, fSection_, pal::Text);
    };
    auto caption = [&](int x, int y, int w, const wchar_t* s) {
        Txt({ x, y, x + w, y + Dp(16) }, s, fTiny_, pal::Text3);
    };

    int y = Dp(lay::FirstPanelY);

    // ---- TARGET ----
    panel(y, Dp(lay::TargetH), L"TARGET");
    {
        const bool hotR = hover_ == HIT_REFRESH;
        FillRound(geo_.refresh, Dpf(6), hotR ? pal::Hover : pal::PanelIn);
        Txt(geo_.refresh, L"REFRESH", fTiny_, hotR ? pal::Text : pal::Text2);
    }
    y += Dp(lay::TargetH + lay::Gap);

    // ---- SCALING ----
    panel(y, Dp(lay::ScaleH), L"SCALING");
    {
        const int p = y;
        caption(mx + Dp(lay::PanelPad), p + Dp(30), mw / 2, L"Algorithm");
        wchar_t v[32];
        swprintf_s(v, L"%.2f\x00D7", double(ui_.scaleFactor));
        caption(mx + Dp(lay::PanelPad), p + Dp(84), mw / 2, L"Scale factor");
        Txt({ geo_.slScale.left, p + Dp(84), geo_.slScale.right, p + Dp(100) }, v, fValue_, pal::Text2);
        DrawSlider(geo_.slScale, MapRange(ui_.scaleFactor, 0.25f, 4.0f), hover_ == HIT_SL_SCALE || drag_ == HIT_SL_SCALE);

        swprintf_s(v, L"%d%%", int(ui_.casAmount * 100.f + 0.5f));
        caption(mx + Dp(lay::PanelPad), p + Dp(132), mw / 2, L"Sharpen before scale (CAS)");
        Txt({ geo_.slCas.left, p + Dp(132), geo_.slCas.right, p + Dp(148) }, v, fValue_, pal::Text2);
        DrawSlider(geo_.slCas, ui_.casAmount, hover_ == HIT_SL_CAS || drag_ == HIT_SL_CAS);
    }
    y += Dp(lay::ScaleH + lay::Gap);

    // ---- FRAME GENERATION ----
    panel(y, Dp(lay::FgH), L"FRAME GENERATION");
    {
        const int p = y;
        const int half = (mw - 2 * Dp(lay::PanelPad) - Dp(12)) / 2;
        caption(mx + Dp(lay::PanelPad), p + Dp(30), half, L"Mode");
        caption(mx + Dp(lay::PanelPad) + half + Dp(12), p + Dp(30), half, L"Flow quality");

        wchar_t v[32];
        swprintf_s(v, L"%d%%", int(ui_.rcasAmount * 100.f + 0.5f));
        caption(mx + Dp(lay::PanelPad), p + Dp(84), mw / 2, L"Sharpen after FG (RCAS)");
        Txt({ geo_.slRcas.left, p + Dp(84), geo_.slRcas.right, p + Dp(100) }, v, fValue_, pal::Text2);
        DrawSlider(geo_.slRcas, ui_.rcasAmount, hover_ == HIT_SL_RCAS || drag_ == HIT_SL_RCAS);

        swprintf_s(v, L"%d%%", int(ui_.artifactSuppression * 100.f + 0.5f));
        caption(mx + Dp(lay::PanelPad), p + Dp(132), mw / 2, L"Artifact suppression");
        Txt({ geo_.slArt.left, p + Dp(132), geo_.slArt.right, p + Dp(148) }, v, fValue_, pal::Text2);
        DrawSlider(geo_.slArt, ui_.artifactSuppression, hover_ == HIT_SL_ART || drag_ == HIT_SL_ART);

        Txt({ mx + Dp(lay::PanelPad), p + Dp(182), geo_.swPred.left - Dp(8), p + Dp(204) },
            L"Predictive motion", fLabel_, pal::Text2);
        DrawSwitch(geo_.swPred, ui_.predictive, hover_ == HIT_SW_PRED);
        Txt({ mx + mw / 2 + Dp(12), p + Dp(182), geo_.swHdr.left - Dp(8), p + Dp(204) },
            L"HDR float pipeline", fLabel_, pal::Text2);
        DrawSwitch(geo_.swHdr, ui_.hdrPass, hover_ == HIT_SW_HDR);
    }
    y += Dp(lay::FgH + lay::Gap);

    // ---- OVERLAY ----
    panel(y, Dp(lay::OverlayH), L"OVERLAY");
    {
        const int p = y;
        Txt({ mx + Dp(lay::PanelPad), p + Dp(34), geo_.swHud.left - Dp(10), p + Dp(58) },
            L"Show FPS overlay \x00B7 BASE + GENERATED", fLabel_, pal::Text2);
        DrawSwitch(geo_.swHud, ui_.showHud, hover_ == HIT_SW_HUD);
        caption(mx + Dp(lay::PanelPad), p + Dp(70), mw - Dp(120), L"FPS cap (0 = display refresh)");
    }
}

// ------------------------------------------------------------ interaction --
int MainWindow::HitTest(int x, int y) const {
    if (InR(geo_.scaleBtn, x, y)) return HIT_SCALE;
    if (InR(geo_.refresh, x, y))  return HIT_REFRESH;
    if (InR(geo_.plus, x, y))     return HIT_PLUS;
    if (InR(geo_.minus, x, y))    return HIT_MINUS;
    if (InR(geo_.shaders, x, y))  return HIT_SHADERS;
    if (InR(geo_.slScale, x, y))  return HIT_SL_SCALE;
    if (InR(geo_.slCas, x, y))    return HIT_SL_CAS;
    if (InR(geo_.slRcas, x, y))   return HIT_SL_RCAS;
    if (InR(geo_.slArt, x, y))    return HIT_SL_ART;
    if (InR(geo_.swHud, x, y))    return HIT_SW_HUD;
    if (InR(geo_.swPred, x, y))   return HIT_SW_PRED;
    if (InR(geo_.swHdr, x, y))    return HIT_SW_HDR;
    RECT crc{};
    GetClientRect(hwnd_, &crc);                          // same cutoff as PaintSidebar
    for (size_t i = 0; i < geo_.profileRows.size(); ++i) {
        if (geo_.profileRows[i].bottom > crc.bottom - Dp(168)) break;
        if (InR(geo_.profileRows[i], x, y)) return HIT_PROFILE + int(i);
    }
    return HIT_NONE;
}

float MainWindow::SliderT(const RECT& track, int x) const {
    const float w = float(track.right - track.left);
    return std::clamp((float(x - track.left)) / w, 0.0f, 1.0f);
}

void MainWindow::SetSliderFromX(int hit, int x) {
    switch (hit) {
    case HIT_SL_SCALE: {
        const float t = SliderT(geo_.slScale, x);
        ui_.scaleFactor = 0.25f + t * (4.0f - 0.25f);
        break;
    }
    case HIT_SL_CAS:  ui_.casAmount = SliderT(geo_.slCas, x); break;
    case HIT_SL_RCAS: ui_.rcasAmount = SliderT(geo_.slRcas, x); break;
    case HIT_SL_ART:  ui_.artifactSuppression = SliderT(geo_.slArt, x); break;
    default: return;
    }
    ApplySettingsLive();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

// --------------------------------------------------------------- data glue --
void MainWindow::SyncComboSelections() {
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_ALGO), int(ui_.algo));
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_FGMODE), int(ui_.fgMode));
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_QUAL), int(ui_.fgQuality));
    SetDlgItemInt(hwnd_, IDC_FPS_CAP, UINT(std::max(0, ui_.fpsCap)), FALSE);
}

void MainWindow::CollectSettings(Settings& s) const {
    s = ui_;
    const int a = ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_ALGO));
    const int m = ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_FGMODE));
    const int q = ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_QUAL));
    if (a >= 0) s.algo = ScaleAlgo(a);
    if (m >= 0) s.fgMode = FgMode(m);
    if (q >= 0) s.fgQuality = FgQuality(q);
    s.fpsCap = int(GetDlgItemInt(hwnd_, IDC_FPS_CAP, nullptr, FALSE));
    s.targetHwnd = nullptr;
}

void MainWindow::ApplySettingsLive() {
    if (building_) return;
    Settings s;
    CollectSettings(s);
    if (Engine::I().Running()) Engine::I().ApplySettings(s);
}

void MainWindow::RefreshTargetList() {
    building_ = true;
    ComboBox_ResetContent(GetDlgItem(hwnd_, IDC_TARGET));
    targets_ = Engine::EnumerateTargets();
    for (const auto& t : targets_) {
        const std::wstring entry = t.title + L"  [" + t.exe + L"]";
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_TARGET), entry.c_str());
    }
    if (!targets_.empty()) ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_TARGET), 0);
    building_ = false;
}

void MainWindow::RefreshProfileRows() {
    profileNames_.assign(1, L"Default");
    for (const auto& p : profiles_.All()) profileNames_.push_back(p.name);
    if (selProfile_ >= int(profileNames_.size())) selProfile_ = 0;
    Layout();
}

void MainWindow::SelectProfile(int idx) {
    if (idx < 0 || idx >= int(profileNames_.size())) return;
    selProfile_ = idx;
    ui_ = idx == 0 ? Settings{} : profiles_.All()[idx - 1].settings;
    building_ = true;
    SyncComboSelections();
    building_ = false;
    ApplySettingsLive();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::SaveCurrentAsProfile() {
    Profile p;
    const int sel = ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_TARGET));
    if (sel >= 0 && sel < int(targets_.size())) {
        p.matchExe = targets_[sel].exe;
        p.matchTitle = targets_[sel].title;
        p.name = targets_[sel].exe;
    } else {
        p.name = L"Profile " + std::to_wstring(profiles_.All().size() + 1);
    }
    CollectSettings(p.settings);
    profiles_.Upsert(p);
    profiles_.Save();
    RefreshProfileRows();
    for (size_t i = 1; i < profileNames_.size(); ++i)
        if (profileNames_[i] == p.name) { selProfile_ = int(i); break; }
    statusText_ = L"Profile saved: " + p.name;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::DeleteSelectedProfile() {
    if (selProfile_ <= 0) {
        statusText_ = L"Select a saved profile first, then \x2013";
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    profiles_.Erase(profileNames_[selProfile_]);
    profiles_.Save();
    selProfile_ = 0;
    ui_ = Settings{};
    RefreshProfileRows();
    building_ = true;
    SyncComboSelections();
    building_ = false;
    statusText_ = L"Profile deleted";
    InvalidateRect(hwnd_, nullptr, FALSE);
}

// ------------------------------------------------------- start/stop + status --
void MainWindow::BeginCountdown() {
    const int sel = ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_TARGET));
    if (sel < 0 || sel >= int(targets_.size())) {
        statusText_ = L"Pick a target window first";
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    countdown_ = 5;                                      // 5 s to focus the game window
    SetTimer(hwnd_, kTimerCount, 1000, nullptr);
    UpdateStatus();
}

void MainWindow::CancelCountdown(const wchar_t* why) {
    KillTimer(hwnd_, kTimerCount);
    countdown_ = 0;
    if (why) statusText_ = why;
    UpdateStatus();
}

void MainWindow::CountTick() {
    --countdown_;
    if (countdown_ <= 0) {
        KillTimer(hwnd_, kTimerCount);
        countdown_ = 0;
        StartEngine();
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
    UpdateStatus();
}

void MainWindow::StartEngine() {
    const int sel = ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_TARGET));
    if (sel < 0 || sel >= int(targets_.size())) {
        statusText_ = L"Pick a target window first";
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    Settings s;
    CollectSettings(s);
    if (!Engine::I().Start(targets_[sel].hwnd, s)) {
        statusText_ = L"Start failed \x2014 capture device or shader error, see flowscale.log";
    }
    UpdateStatus();
}

void MainWindow::StopEngine() {
    Engine::I().Stop();
    UpdateStatus();
}

void MainWindow::UpdateStatus() {
    running_ = Engine::I().Running();
    if (countdown_ > 0) {
        statusText_ = L"Starting in " + std::to_wstring(countdown_)
                    + L" \x2014 focus the game window (click SCALE to cancel)";
    } else if (running_) {
        const HudState h = Engine::I().stats.Snapshot();
        wchar_t buf[160];
        swprintf_s(buf, L"Running \x00B7 in %.0f fps \x00B7 generated %.0f fps \x00B7 p50 %.1f ms",
                   double(h.srcFps), double(h.genFps), double(h.latencyP50));
        statusText_ = buf;
    } else {
        statusText_ = L"Stopped \x00B7 pick a window, adjust, then SCALE (5 s head-start)";
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::OpenShaderFolder() {
    const std::wstring dir = ShaderDir() + L"\\user";
    CreateDirectoryW(dir.c_str(), nullptr);
    ShellExecuteW(hwnd_, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

} // namespace fs
