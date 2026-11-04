#include "capture/DDACapture.h"
#include "util/Log.h"

namespace fs {

bool DDACapture::Start(Device& dev, HWND hwnd) {
    HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!mon) { LOGE("DDA: no monitor for target"); return false; }

    // Find the adapter output that owns the target monitor.
    CPtr<IDXGIOutput> found;
    CPtr<IDXGIAdapter1> adapter = dev.Adapter();
    for (UINT i = 0; ; ++i) {
        CPtr<IDXGIOutput> out;
        if (adapter->EnumOutputs(i, out.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_OUTPUT_DESC d{};
        if (SUCCEEDED(out->GetDesc(&d)) && d.Monitor == mon) { found = std::move(out); break; }
    }
    if (!found) { LOGE("DDA: target monitor not on the FlowScale adapter"); return false; }

    CPtr<IDXGIOutput1> out1;
    ThrowIfFailed(found->QueryInterface(IID_PPV_ARGS(out1.GetAddressOf())), "QI IDXGIOutput1");

    dup_.Reset();
    ThrowIfFailed(out1->DuplicateOutput(dev.Dev(), dup_.GetAddressOf()), "DuplicateOutput");

    DXGI_OUTDUPL_DESC dd{};
    dup_->GetDesc(&dd);
    width_ = dd.ModeDesc.Width; height_ = dd.ModeDesc.Height;

    // Scratch texture used when the engine has no staging destination (kept for
    // completeness; the engine always passes its history texture as `out`).
    D3D11_TEXTURE2D_DESC td{};
    td.Width = width_; td.Height = height_;
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = dd.ModeDesc.Format;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ThrowIfFailed(dev.Dev()->CreateTexture2D(&td, nullptr, staged_.GetAddressOf()), "DDA staged tex");

    ctx_ = dev.Ctx();   // borrowed; PollFrame runs on the present thread that owns it
    haveFrame_ = false;
    LOGI("DDA capturing monitor %ux%u", width_, height_);
    return true;
}

bool DDACapture::PollFrame(ID3D11Texture2D* out) {
    if (!dup_ || !ctx_) return false;

    DXGI_OUTDUPL_FRAME_INFO info{};
    CPtr<IDXGIResource> res;
    HRESULT hr = dup_->AcquireNextFrame(0, &info, res.GetAddressOf());
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return haveFrame_ && false;  // nothing new
    if (hr == DXGI_ERROR_ACCESS_LOST) {        // mode change / fullscreen transition
        LOGW("DDA access lost; engine will restart capture");
        dup_.Reset();
        return false;
    }
    ThrowIfFailed(hr, "AcquireNextFrame");

    if (info.LastPresentTime.QuadPart != 0 || info.AccumulatedFrames > 0) {
        CPtr<ID3D11Texture2D> desktop;
        if (SUCCEEDED(res->QueryInterface(IID_PPV_ARGS(desktop.GetAddressOf())))) {
            ctx_->CopyResource(out ? out : staged_.Get(), desktop.Get());
            haveFrame_ = true;
        }
    }
    ThrowIfFailed(dup_->ReleaseFrame(), "ReleaseFrame");
    return haveFrame_;
}

void DDACapture::Stop() {
    dup_.Reset();
    staged_.Reset();
    ctx_ = nullptr;
    haveFrame_ = false;
}

} // namespace fs
