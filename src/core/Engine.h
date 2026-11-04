// FlowScale — engine orchestration: capture -> pace -> framegen -> scale -> present.
//
// Thread rules (docs/ARCHITECTURE.md):
//   * the present thread owns the D3D11 immediate context and every pass;
//   * the UI thread only mutates an atomic settings snapshot and start/stop flags;
//   * WGC delivers surfaces on threadpool threads — we only store refs there.
#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>
#include "capture/ICapture.h"
#include "core/PacerMath.h"
#include "core/Settings.h"
#include "core/Stats.h"
#include "framegen/FrameGen.h"
#include "gfx/Device.h"
#include "gfx/Hud.h"
#include "gfx/Presenter.h"
#include "scaling/ScalerChain.h"

namespace fs {

struct TargetWindow {
    HWND        hwnd = nullptr;
    std::wstring title;
    std::wstring exe;
};

class Engine {
public:
    static Engine& I();

    bool Start(HWND target, const Settings& s);
    void Stop();
    bool Running() const { return running_.load(std::memory_order_relaxed); }

    // UI thread: swap the settings snapshot; picked up on the next present tick.
    void ApplySettings(const Settings& s);
    Settings CurrentSettings() const;

    // Window picker data.
    static std::vector<TargetWindow> EnumerateTargets();

    bool TargetClosed() const;

    Stats stats;

private:
    Engine() = default;
    void PresentLoop(std::stop_token st);
    void ApplyLive(const Settings& s);          // present-thread side
    TickDecision TickPacer(double now);         // mutex-guarded pacer read
    double pacerLatestArrival();                // mutex-guarded pacer read
    bool EnsureFrameGen(const Settings& s);     // lazy (re)init at capture size

    Device            dev_;
    std::unique_ptr<ICapture> capture_;
    Presenter         presenter_;
    Hud               hud_;
    ScalerChain       scaler_;
    FrameGen          frameGen_;

    CPtr<ID3D11Texture2D> realTex_;   // latest real frame at capture size (BGRA)
    CPtr<ID3D11Texture2D> finalTex_;  // scaler output at monitor size
    uint32_t capW_ = 0, capH_ = 0;

    std::jthread      presentThread_;
    std::atomic<bool> running_{ false };
    mutable std::mutex settingsMtx_;
    Settings          settings_;
    PacerMath         pacer_{ 1.0 };
};

} // namespace fs
