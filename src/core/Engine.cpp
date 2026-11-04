#include "core/Engine.h"
#include "capture/DDACapture.h"
#include "capture/WGCCapture.h"
#include "util/Common.h"
#include "util/Log.h"
#include "util/Timer.h"
#include <dwmapi.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dwmapi.lib")

namespace fs {

Engine& Engine::I() {
    static Engine e;
    return e;
}

// ---------------------------------------------------------------- targets ----
static std::wstring ExeOfWindow(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return {};
    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc) return {};
    wchar_t path[MAX_PATH]{};
    DWORD len = MAX_PATH;
    std::wstring full;
    if (QueryFullProcessImageNameW(proc, 0, path, &len)) full = path;
    CloseHandle(proc);
    const size_t slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? full : full.substr(slash + 1);
}

std::vector<TargetWindow> Engine::EnumerateTargets() {
    std::vector<TargetWindow> out;
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* list = reinterpret_cast<std::vector<TargetWindow>*>(lp);
        if (!IsWindowVisible(h)) return TRUE;
        if (GetWindowLongW(h, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
        // Skip cloaked UWP ghosts.
        BOOL cloaked = FALSE;
        if (SUCCEEDED(DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked)
            return TRUE;
        wchar_t title[256]{};
        if (!GetWindowTextW(h, title, 256)) return TRUE;
        if (wcslen(title) < 2) return TRUE;
        list->push_back({ h, title, ExeOfWindow(h) });
        return TRUE;
    }, reinterpret_cast<LPARAM>(&out));

    std::sort(out.begin(), out.end(), [](const TargetWindow& a, const TargetWindow& b) {
        return a.title < b.title;
    });
    return out;
}

// --------------------------------------------------------------- lifecycle ----
bool Engine::Start(HWND target, const Settings& s) {
    if (running_) return true;
    if (!dev_.IsValid() && !dev_.Init(false)) return false;
    settings_ = s;

    // 1) Capture backend.
    capture_ = CreateCapture(dev_, target);
    if (!capture_) {
        // Desktop/monitor target or WGC unavailable -> Desktop Duplication.
        capture_ = std::make_unique<DDACapture>();
    }
    if (!capture_->Start(dev_, target)) { LOGE("capture start failed"); capture_.reset(); return false; }
    capture_->onFrame = [this](ID3D11Texture2D* surf, double t) {
        // Threadpool thread: record arrival only; the present thread pulls the surface.
        UNREFERENCED_PARAMETER(surf); UNREFERENCED_PARAMETER(t);
    };
    capW_ = capture_->Width(); capH_ = capture_->Height();
    if (!capW_ || !capH_) { LOGE("capture reported empty size"); Stop(); return false; }

    // 2) Presenter on the target's monitor.
    HMONITOR mon = MonitorFromWindow(target, MONITOR_DEFAULTTONEAREST);
    if (!presenter_.Init(dev_, mon, /*tryHdr*/ false)) { Stop(); return false; }
    presenter_.SetHud(&hud_);
    if (!hud_.Init(dev_, presenter_.Width(), presenter_.Height(), DXGI_FORMAT_B8G8R8A8_UNORM))
        LOGW("HUD init failed; continuing without overlay");

    // 3) Real-frame + final textures.
    realTex_ = dev_.CreateTexture(capW_, capH_, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_BIND_SHADER_RESOURCE);
    finalTex_ = dev_.CreateTexture(presenter_.Width(), presenter_.Height(),
                                   DXGI_FORMAT_B8G8R8A8_UNORM,
                                   D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);

    // 4) Scaler + (optional) frame gen.
    if (!scaler_.Init(dev_, settings_, capW_, capH_, presenter_.Width(), presenter_.Height(),
                      DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM)) {
        LOGE("scaler init failed: %ls", scaler_.StatusText().c_str());
        Stop(); return false;
    }
    if (settings_.fgMode != FgMode::Off && !EnsureFrameGen(settings_)) {
        LOGE("framegen init failed; running without FG");
    }

    // 5) Pacer + present thread.
    { std::lock_guard<std::mutex> g(settingsMtx_); pacer_ = PacerMath(s.predictive ? 1.5 : 1.0); }
    stats = Stats{};
    running_ = true;
    presentThread_ = std::jthread([this](std::stop_token st) { PresentLoop(st); });

    LOGI("Engine started: %ux%u -> %ux%u, FG=%d", capW_, capH_,
         presenter_.Width(), presenter_.Height(), int(settings_.fgMode != FgMode::Off));
    return true;
}

void Engine::Stop() {
    if (presentThread_.joinable()) {
        presentThread_.request_stop();
        presentThread_.join();
    }
    running_ = false;
    frameGen_.Shutdown();
    scaler_.Shutdown();
    hud_.Shutdown();
    presenter_.Shutdown();
    if (capture_) { capture_->Stop(); capture_.reset(); }
    realTex_.Reset(); finalTex_.Reset();
    LOGI("Engine stopped");
}

bool Engine::TargetClosed() const {
    if (auto* wgc = dynamic_cast<WGCCapture*>(capture_.get())) return wgc->TargetClosed();
    return false;
}

void Engine::ApplySettings(const Settings& s) {
    std::lock_guard<std::mutex> g(settingsMtx_);
    settings_ = s;
    pacer_ = PacerMath(s.predictive ? 1.5 : 1.0);
}

Settings Engine::CurrentSettings() const {
    std::lock_guard<std::mutex> g(settingsMtx_);
    return settings_;
}

TickDecision Engine::TickPacer(double now) {
    std::lock_guard<std::mutex> g(settingsMtx_);
    return pacer_.Tick(now);
}

double Engine::pacerLatestArrival() {
    std::lock_guard<std::mutex> g(settingsMtx_);
    return pacer_.LatestArrival();
}

// ------------------------------------------------------------ framegen mgmt ----
bool Engine::EnsureFrameGen(const Settings& s) {
    if (s.fgMode == FgMode::Off) return false;
    if (frameGen_.Width() == capW_ && frameGen_.Height() == capH_ && frameGen_.Ready())
        return true;
    if (frameGen_.Width() != 0 && (frameGen_.Width() != capW_ || frameGen_.Height() != capH_))
        frameGen_.Shutdown();
    if (frameGen_.Width() == 0) {
        if (!frameGen_.Init(dev_, capW_, capH_, s.fgQuality, s.artifactSuppression)) return false;
    }
    frameGen_.ApplySettings(s.fgQuality, s.artifactSuppression);
    return true;
}

// ------------------------------------------------------------- present loop ----
void Engine::PresentLoop(std::stop_token st) {
    const double minInterval = settings_.fpsCap > 0 ? 1.0 / settings_.fpsCap : 0.0;
    double lastRealArrival = 0.0;
    double lastStatUpdate = 0.0;
    uint32_t presents = 0, gens = 0;

    while (!st.stop_requested()) {
        const double now = presenter_.WaitTick(minInterval);

        // --- drain the newest capture frame -------------------------------
        bool newReal = false;
        if (auto* wgc = dynamic_cast<WGCCapture*>(capture_.get())) {
            double tArr = 0.0;
            CPtr<ID3D11Texture2D> surf = wgc->TakePendingSurface(&tArr);
            if (surf) {
                dev_.Ctx()->CopyResource(realTex_.Get(), surf.Get());
                lastRealArrival = tArr;
                newReal = true;
            }
        } else if (capture_ && capture_->IsMonitor()) {
            if (capture_->PollFrame(realTex_.Get())) {
                lastRealArrival = Timer::Now();
                newReal = true;
            }
        }

        const Settings s = [this] { std::lock_guard<std::mutex> g(settingsMtx_); return settings_; }();

        if (newReal) {
            { std::lock_guard<std::mutex> g(settingsMtx_); pacer_.OnSourceFrame(lastRealArrival); }
            stats.OnSourceFrame(lastRealArrival);
            if (s.fgMode != FgMode::Off) {
                ApplyLive(s);
                if (EnsureFrameGen(s)) frameGen_.OnNewRealFrame(realTex_.Get());
            }
        }

        // --- decide what this display tick shows ---------------------------
        const TickDecision d = TickPacer(now);
        ID3D11Texture2D* frame = nullptr;
        if (d.kind == TickDecision::Kind::Real) {
            frame = realTex_.Get();
        } else if (d.kind == TickDecision::Kind::Generated) {
            if (s.fgMode != FgMode::Off && frameGen_.Ready()) {
                frame = frameGen_.Generate(d.t);
                ++gens;
            } else {
                frame = realTex_.Get();     // hold last real while FG warms up
            }
        } else if (pacerLatestArrival() > 0.0) {
            frame = realTex_.Get();         // single-frame hold (first ticks)
        }
        if (!frame) continue;

        // --- scale + present -----------------------------------------------
        scaler_.Process(frame, finalTex_.Get());
        const double submit = Timer::Now();
        if (lastRealArrival > 0.0) stats.OnFrameLatency(submit - lastRealArrival);
        stats.OnPresent(submit);
        ++presents;

        if (s.showHud) {
            HudState hud = stats.Snapshot();
            hud.algoLabel  = Settings::AlgoName(s.algo);
            hud.fgLabel    = Settings::FgModeName(s.fgMode);
            presenter_.Present(frame, true, &hud);
        } else {
            presenter_.Present(frame, true, nullptr);
        }

        // --- ~1 Hz stats update ---------------------------------------------
        if (now - lastStatUpdate >= 1.0) {
            double span = 0.0;
            { std::lock_guard<std::mutex> g(settingsMtx_); span = pacer_.SegmentSpan(); }
            stats.SetSrcFps(span > 1e-3 ? float(1.0 / span) : 0.f);
            stats.SetOutFps(float(presents));
            presents = 0; gens = 0;
            lastStatUpdate = now;
            if (TargetClosed()) {
                LOGI("target closed; present loop exits (UI thread will finalize)");
                break;   // never Stop() from this thread: it would join itself
            }
        }
    }
}

// Keeps the scaler chain in sync with settings changes (algorithm, sharpening...).
void Engine::ApplyLive(const Settings& s) {
    scaler_.ApplySettings(dev_, s, capW_, capH_, presenter_.Width(), presenter_.Height());
    if (s.algo == ScaleAlgo::UserShader) scaler_.HotReloadUserShader(dev_.Dev());
}

} // namespace fs
