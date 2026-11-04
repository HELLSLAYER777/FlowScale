// FlowScale — Direct2D HUD overlay (FPS, latency percentiles, mode labels).
#pragma once
#include <d2d1_1.h>
#include <dwrite.h>
#include "gfx/Device.h"
#include "core/Stats.h"

namespace fs {

class Hud {
public:
    bool Init(Device& dev, uint32_t width, uint32_t height, DXGI_FORMAT fmt);
    void Shutdown();
    void Resize(uint32_t width, uint32_t height, ID3D11Texture2D* backbuffer);

    // Draws the HUD onto `target` (the backbuffer). Cheap enough to call every frame.
    void Draw(ID3D11Texture2D* target, const HudState& s);

private:
    void EnsureBitmap(ID3D11Texture2D* backbuffer);

    Device* dev_ = nullptr;
    uint32_t w_ = 0, h_ = 0;

    CPtr<ID2D1Factory1>       factory_;   // Factory1: CreateDevice(DXGI) needs D2D 1.1
    CPtr<ID2D1Device>         d2dDev_;
    CPtr<ID2D1DeviceContext>  ctx_;
    CPtr<ID2D1Bitmap1>        bitmap_;
    CPtr<IDWriteFactory>      dwrite_;
    CPtr<IDWriteTextFormat>   fmtBig_, fmtSmall_;
    CPtr<ID2D1SolidColorBrush> brushWhite_, brushAccent_;
};

} // namespace fs
