#include "app/MainWindow.h"
#include "util/Common.h"
#include "util/Log.h"
#include "util/Timer.h"
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <dwmapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")

namespace fs {

// Control IDs
enum {
    IDC_TARGET = 101, IDC_ALGO, IDC_FGMODE, IDC_QUAL, IDC_SCALE, IDC_SCALE_SLIDER,
    IDC_HUD, IDC_PRED, IDC_HDR, IDC_FPS_CAP, IDC_CAS, IDC_RCAS, IDC_ARTIFACT,
    IDC_START, IDC_STOP, IDC_SAVE_PROFILE, IDC_OPEN_SHADERS, IDC_REFRESH, IDC_STATUS,
    IDC_LBL0
};

// Controls default to the bitmap System font, which looks broken under
// per-monitor-v2 DPI awareness; give every control a Segoe UI font instead.
static HFONT UiFont() {
    static HFONT font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    return font;
}

static HWND MkCtl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style,
                  int id, int x, int y, int w, int h) {
    HWND ctl = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                               x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<intptr_t>(id)),
                               GetModuleHandleW(nullptr), nullptr);
    if (ctl) SendMessageW(ctl, WM_SETFONT, reinterpret_cast<WPARAM>(UiFont()), TRUE);
    return ctl;
}

static void DarkTheme(HWND h) {
    BOOL on = TRUE;
    DwmSetWindowAttribute(h, DWMWA_USE_IMMERSIVE_DARK_MODE, &on, sizeof(on));
}

bool MainWindow::Create(HINSTANCE hInst) {
    inst_ = hInst;
    Timer::Init();

    // Load comctl32 and register the common-control classes (trackbars etc.);
    // without this the DLL may never load and msctls_trackbar32 stays unregistered.
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(24, 26, 32));
    wc.lpszClassName = kClass;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);   // shared icon: (hInst, IDI_APPLICATION) never resolves
    RegisterClassExW(&wc);

    profiles_.Load();

    constexpr int W = 380, H = 560;
    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kClass, L"FlowScale",
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT, W, H,
                            nullptr, nullptr, hInst, this);
    if (!hwnd_) return false;

    DarkTheme(hwnd_);
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
    case WM_CREATE:
        // WM_CREATE is dispatched from INSIDE CreateWindowExW in MainWindow::Create,
        // i.e. before hwnd_ is assigned. Without storing `h` here every child
        // control below would be created with a NULL parent (fatal for WS_CHILD:
        // CreateWindowExW fails silently) and the window would come up empty.
        hwnd_ = h;
        BuildControls();
        // Global hotkeys: Ctrl+Alt+S start/stop, Ctrl+Alt+G cycle FG, Ctrl+Alt+H HUD.
        RegisterHotKey(h, 1, MOD_CONTROL | MOD_ALT, 'S');
        RegisterHotKey(h, 2, MOD_CONTROL | MOD_ALT, 'G');
        RegisterHotKey(h, 3, MOD_CONTROL | MOD_ALT, 'H');
        SetTimer(h, refreshTimer_ = 4, 1000, nullptr);
        return 0;

    case WM_TIMER:
        if (wp == 4) UpdateStatus();
        return 0;

    case WM_HOTKEY: {
        Settings s = Engine::I().CurrentSettings();
        if (wp == 1) {                                     // start/stop toggle
            if (Engine::I().Running()) Engine::I().Stop();
            else {
                const int sel = ComboBox_GetCurSel(GetDlgItem(h, IDC_TARGET));
                if (sel >= 0 && sel < int(targets_.size()))
                    Engine::I().Start(targets_[sel].hwnd, s);
            }
        } else if (wp == 2) {                              // cycle FG mode
            s.fgMode = FgMode((int(s.fgMode) + 1) % 4);
            if (Engine::I().Running()) Engine::I().ApplySettings(s);
            SettingsToUI(s);
        } else if (wp == 3) {                              // toggle HUD
            s.showHud = !s.showHud;
            if (Engine::I().Running()) Engine::I().ApplySettings(s);
            CheckDlgButton(h, IDC_HUD, s.showHud ? BST_CHECKED : BST_UNCHECKED);
        }
        UpdateStatus();
        return 0;
    }

    case WM_COMMAND: {
        const int id = LOWORD(wp);
        const int code = HIWORD(wp);
        if (id == IDC_START && code == BN_CLICKED) {
            const int sel = ComboBox_GetCurSel(GetDlgItem(h, IDC_TARGET));
            if (sel < 0 || sel >= int(targets_.size())) {
                SetDlgItemTextW(h, IDC_STATUS, L"Pick a target window first");
                return 0;
            }
            Settings s;
            SettingsFromUI(s);
            // Auto-match profile by exe/title.
            const auto& t = targets_[sel];
            if (auto p = profiles_.Match(t.exe, t.title)) SettingsToUI(p->settings), SettingsFromUI(s);
            Engine::I().Start(t.hwnd, s);
            UpdateStatus();
        } else if (id == IDC_STOP && code == BN_CLICKED) {
            Engine::I().Stop();
            UpdateStatus();
        } else if (id == IDC_SAVE_PROFILE && code == BN_CLICKED) {
            const int sel = ComboBox_GetCurSel(GetDlgItem(h, IDC_TARGET));
            Profile p;
            p.name = sel >= 0 && sel < int(targets_.size())
                   ? targets_[sel].exe : L"manual";
            if (sel >= 0 && sel < int(targets_.size())) {
                p.matchExe = targets_[sel].exe;
                p.matchTitle = targets_[sel].title;
            }
            SettingsFromUI(p.settings);
            profiles_.Upsert(p);
            profiles_.Save();
            RefreshProfileList();
            SetDlgItemTextW(h, IDC_STATUS, (L"Profile saved: " + p.name).c_str());
        } else if (id == IDC_OPEN_SHADERS && code == BN_CLICKED) {
            const std::wstring dir = ShaderDir() + L"\\user";
            CreateDirectoryW(dir.c_str(), nullptr);
            ShellExecuteW(h, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else if (id == IDC_REFRESH && code == BN_CLICKED) {
            RefreshTargetList();
        } else if (id == IDC_ALGO && code == CBN_SELCHANGE) {
            // User shader choice needs the shader-folder contents; keep simple combo.
            ApplySettingsLive();
        } else if (code == CBN_SELCHANGE || code == BN_CLICKED || code == EN_CHANGE) {
            ApplySettingsLive();
        }
        return 0;
    }

    case WM_HSCROLL:
        ApplySettingsLive();
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, RGB(235, 235, 240));
        SetBkColor(dc, RGB(24, 26, 32));
        static HBRUSH bg = CreateSolidBrush(RGB(24, 26, 32));
        return reinterpret_cast<LRESULT>(bg);
    }

    case WM_DESTROY:
        KillTimer(h, 4);
        UnregisterHotKey(h, 1); UnregisterHotKey(h, 2); UnregisterHotKey(h, 3);
        Engine::I().Stop();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

void MainWindow::BuildControls() {
    const wchar_t* labelCls = L"STATIC";
    const int X = 16, W = 340, LH = 20;

    MkCtl(hwnd_, labelCls, L"Target window", 0, IDC_LBL0, X, 10, W, LH);
    MkCtl(hwnd_, L"COMBOBOX", nullptr, DWORD(CBS_DROPDOWNLIST) | DWORD(WS_VSCROLL), IDC_TARGET, X, 30, 290, 300);
    MkCtl(hwnd_, L"BUTTON", L"...", BS_PUSHBUTTON, IDC_REFRESH, X + 296, 30, 44, 24);

    MkCtl(hwnd_, labelCls, L"Scale factor", 0, IDC_LBL0, X, 62, W, LH);
    MkCtl(hwnd_, L"EDIT", L"1.50", WS_BORDER | ES_CENTER, IDC_SCALE, X, 82, 70, 22);
    MkCtl(hwnd_, L"msctls_trackbar32", nullptr, TBS_HORZ | TBS_AUTOTICKS, IDC_SCALE_SLIDER,
          X + 80, 82, 260, 24);
    SendDlgItemMessageW(hwnd_, IDC_SCALE_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(25, 400));

    MkCtl(hwnd_, labelCls, L"Algorithm", 0, IDC_LBL0, X, 112, W, LH);
    MkCtl(hwnd_, L"COMBOBOX", nullptr, CBS_DROPDOWNLIST, IDC_ALGO, X, 132, W, 200);

    MkCtl(hwnd_, labelCls, L"Frame generation", 0, IDC_LBL0, X, 162, W, LH);
    MkCtl(hwnd_, L"COMBOBOX", nullptr, CBS_DROPDOWNLIST, IDC_FGMODE, X, 182, W, 120);

    MkCtl(hwnd_, labelCls, L"Flow quality", 0, IDC_LBL0, X, 212, W, LH);
    MkCtl(hwnd_, L"COMBOBOX", nullptr, CBS_DROPDOWNLIST, IDC_QUAL, X, 232, W, 120);

    MkCtl(hwnd_, labelCls, L"Sharpen before scale (CAS)", 0, IDC_LBL0, X, 262, W, LH);
    MkCtl(hwnd_, L"msctls_trackbar32", nullptr, TBS_HORZ, IDC_CAS, X, 280, W, 24);
    MkCtl(hwnd_, labelCls, L"Sharpen after framegen (RCAS)", 0, IDC_LBL0, X, 306, W, LH);
    MkCtl(hwnd_, L"msctls_trackbar32", nullptr, TBS_HORZ, IDC_RCAS, X, 324, W, 24);
    MkCtl(hwnd_, labelCls, L"Artifact suppression", 0, IDC_LBL0, X, 350, W, LH);
    MkCtl(hwnd_, L"msctls_trackbar32", nullptr, TBS_HORZ, IDC_ARTIFACT, X, 368, W, 24);

    MkCtl(hwnd_, L"BUTTON", L"Show latency HUD", BS_AUTOCHECKBOX, IDC_HUD, X, 398, 160, 20);
    MkCtl(hwnd_, L"BUTTON", L"Predictive motion", BS_AUTOCHECKBOX, IDC_PRED, X + 165, 398, 160, 20);
    MkCtl(hwnd_, L"BUTTON", L"HDR float pipeline (experimental)", BS_AUTOCHECKBOX, IDC_HDR, X, 420, 240, 20);

    MkCtl(hwnd_, labelCls, L"FPS cap (0 = display refresh)", 0, IDC_LBL0, X + 165, 420, 170, LH);
    MkCtl(hwnd_, L"EDIT", L"0", WS_BORDER | ES_NUMBER, IDC_FPS_CAP, X + 300, 438, 40, 20);

    MkCtl(hwnd_, L"BUTTON", L"Start", BS_PUSHBUTTON, IDC_START, X, 462, 80, 28);
    MkCtl(hwnd_, L"BUTTON", L"Stop", BS_PUSHBUTTON, IDC_STOP, X + 86, 462, 80, 28);
    MkCtl(hwnd_, L"BUTTON", L"Save profile", BS_PUSHBUTTON, IDC_SAVE_PROFILE, X + 172, 462, 84, 28);
    MkCtl(hwnd_, L"BUTTON", L"Shader folder (F5 reloads)", BS_PUSHBUTTON, IDC_OPEN_SHADERS, X + 262, 462, 78, 28);

    MkCtl(hwnd_, L"STATIC", L"Stopped — hotkeys: Ctrl+Alt+S start/stop, Ctrl+Alt+G cycle FG, Ctrl+Alt+H HUD",
          0, IDC_STATUS, X, 498, W, 40);

    // Populate combos.
    for (int a = 0; a <= 5; ++a)
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_ALGO), Settings::AlgoName(ScaleAlgo(a)));
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_ALGO), int(ScaleAlgo::Lanczos3));
    for (int m = 0; m <= 3; ++m)
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_FGMODE), Settings::FgModeName(FgMode(m)));
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_FGMODE), int(FgMode::Off));
    for (int q = 0; q <= 2; ++q)
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_QUAL), Settings::FgQualityName(FgQuality(q)));
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_QUAL), int(FgQuality::Balanced));

    SendDlgItemMessageW(hwnd_, IDC_CAS, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendDlgItemMessageW(hwnd_, IDC_CAS, TBM_SETPOS, TRUE, 30);
    SendDlgItemMessageW(hwnd_, IDC_RCAS, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendDlgItemMessageW(hwnd_, IDC_RCAS, TBM_SETPOS, TRUE, 40);
    SendDlgItemMessageW(hwnd_, IDC_ARTIFACT, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendDlgItemMessageW(hwnd_, IDC_ARTIFACT, TBM_SETPOS, TRUE, 50);
    CheckDlgButton(hwnd_, IDC_HUD, BST_CHECKED);

    RefreshTargetList();
    RefreshProfileList();
}

void MainWindow::RefreshTargetList() {
    building_ = true;
    ComboBox_ResetContent(GetDlgItem(hwnd_, IDC_TARGET));
    targets_ = Engine::EnumerateTargets();
    for (const auto& t : targets_) {
        std::wstring entry = t.title + L"  [" + t.exe + L"]";
        ComboBox_AddString(GetDlgItem(hwnd_, IDC_TARGET), entry.c_str());
    }
    building_ = false;
}

void MainWindow::RefreshProfileList() {
    // Profiles auto-apply on Start; surface the count in the title.
    const int n = int(profiles_.All().size());
    SetWindowTextW(hwnd_, (L"FlowScale — " + std::to_wstring(n) + L" profiles").c_str());
}

void MainWindow::SettingsFromUI(Settings& s) const {
    wchar_t buf[64]{};
    GetDlgItemTextW(hwnd_, IDC_SCALE, buf, 64);
    s.scaleFactor = float(_wtof(buf));
    if (s.scaleFactor < 0.25f) s.scaleFactor = 0.25f;
    if (s.scaleFactor > 4.0f) s.scaleFactor = 4.0f;
    s.algo = ScaleAlgo(std::max(0, ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_ALGO))));
    s.fgMode = FgMode(std::max(0, ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_FGMODE))));
    s.fgQuality = FgQuality(std::max(0, ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_QUAL))));
    s.casAmount = SendDlgItemMessageW(hwnd_, IDC_CAS, TBM_GETPOS, 0, 0) / 100.0f;
    s.rcasAmount = SendDlgItemMessageW(hwnd_, IDC_RCAS, TBM_GETPOS, 0, 0) / 100.0f;
    s.artifactSuppression = SendDlgItemMessageW(hwnd_, IDC_ARTIFACT, TBM_GETPOS, 0, 0) / 100.0f;
    s.showHud = IsDlgButtonChecked(hwnd_, IDC_HUD) == BST_CHECKED;
    s.predictive = IsDlgButtonChecked(hwnd_, IDC_PRED) == BST_CHECKED;
    s.hdrPass = IsDlgButtonChecked(hwnd_, IDC_HDR) == BST_CHECKED;
    s.fpsCap = GetDlgItemInt(hwnd_, IDC_FPS_CAP, nullptr, FALSE);
    s.targetHwnd = nullptr;
}

void MainWindow::SettingsToUI(const Settings& s) {
    building_ = true;
    wchar_t buf[32];
    swprintf_s(buf, L"%.2f", double(s.scaleFactor));
    SetDlgItemTextW(hwnd_, IDC_SCALE, buf);
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_ALGO), int(s.algo));
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_FGMODE), int(s.fgMode));
    ComboBox_SetCurSel(GetDlgItem(hwnd_, IDC_QUAL), int(s.fgQuality));
    SendDlgItemMessageW(hwnd_, IDC_CAS, TBM_SETPOS, TRUE, int(s.casAmount * 100));
    SendDlgItemMessageW(hwnd_, IDC_RCAS, TBM_SETPOS, TRUE, int(s.rcasAmount * 100));
    SendDlgItemMessageW(hwnd_, IDC_ARTIFACT, TBM_SETPOS, TRUE, int(s.artifactSuppression * 100));
    CheckDlgButton(hwnd_, IDC_HUD, s.showHud ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd_, IDC_PRED, s.predictive ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd_, IDC_HDR, s.hdrPass ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(hwnd_, IDC_FPS_CAP, UINT(std::max(0, s.fpsCap)), FALSE);
    building_ = false;
}

void MainWindow::ApplySettingsLive() {
    if (building_) return;
    Settings s;
    SettingsFromUI(s);
    if (Engine::I().Running()) Engine::I().ApplySettings(s);
}

void MainWindow::UpdateStatus() {
    if (!Engine::I().Running()) {
        statusText_ = L"Stopped";
        SetDlgItemTextW(hwnd_, IDC_STATUS, statusText_.c_str());
        return;
    }
    const HudState h = Engine::I().stats.Snapshot();
    wchar_t buf[160];
    swprintf_s(buf, L"Running — in %.0f / out %.0f fps, FG %ls, latency p50 %.1f ms",
               double(h.srcFps), double(h.outFps), h.fgLabel.c_str(), double(h.latencyP50));
    statusText_ = buf;
    SetDlgItemTextW(hwnd_, IDC_STATUS, buf);
}

} // namespace fs
