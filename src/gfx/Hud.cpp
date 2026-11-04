#include "gfx/Hud.h"
#include "util/Log.h"
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace fs {

bool Hud::Init(Device& dev, uint32_t width, uint32_t height, DXGI_FORMAT fmt) {
    dev_ = &dev; w_ = width; h_ = height;

    ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                    factory_.GetAddressOf()), "D2D1CreateFactory");

    // D2D shares the D3D11 device via DXGI.
    CPtr<IDXGIDevice> dxgiDev;
    ThrowIfFailed(dev.Dev()->QueryInterface(IID_PPV_ARGS(dxgiDev.GetAddressOf())), "QI dxgidevice");
    ThrowIfFailed(factory_->CreateDevice(dxgiDev.Get(), d2dDev_.GetAddressOf()), "D2D CreateDevice");
    ThrowIfFailed(d2dDev_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                               ctx_.GetAddressOf()), "D2D CreateDeviceContext");

    ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                      reinterpret_cast<IUnknown**>(dwrite_.GetAddressOf())),
                  "DWriteCreateFactory");

    const D2D1_SIZE_F size = { float(width), float(height) };
    ThrowIfFailed(dwrite_->CreateTextFormat(L"Segoe UI", nullptr,
                 DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
                 DWRITE_FONT_STRETCH_NORMAL, 26.0f, L"en-us", fmtBig_.GetAddressOf()), "TextFormat big");
    ThrowIfFailed(dwrite_->CreateTextFormat(L"Segoe UI", nullptr,
                 DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                 DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", fmtSmall_.GetAddressOf()), "TextFormat small");
    ThrowIfFailed(dwrite_->CreateTextFormat(L"Segoe UI", nullptr,
                 DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
                 DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", fmtTiny_.GetAddressOf()), "TextFormat tiny");

    ThrowIfFailed(ctx_->CreateSolidColorBrush(D2D1::ColorF(0xF2F2F2, 1.0f), brushWhite_.GetAddressOf()), "Brush");
    ThrowIfFailed(ctx_->CreateSolidColorBrush(D2D1::ColorF(0x4FC3F7, 1.0f), brushAccent_.GetAddressOf()), "Brush accent");
    ThrowIfFailed(ctx_->CreateSolidColorBrush(D2D1::ColorF(0xAEB2BC, 1.0f), brushLabel_.GetAddressOf()), "Brush label");

    UNREFERENCED_PARAMETER(fmt);
    return true;
}

void Hud::EnsureBitmap(ID3D11Texture2D* backbuffer) {
    bitmap_.Reset();
    D3D11_TEXTURE2D_DESC d{};
    backbuffer->GetDesc(&d);
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(d.Format, D2D1_ALPHA_MODE_IGNORE));
    CPtr<IDXGISurface> surf;
    if (FAILED(backbuffer->QueryInterface(IID_PPV_ARGS(surf.GetAddressOf())))) return;
    if (FAILED(ctx_->CreateBitmapFromDxgiSurface(surf.Get(), &bp, bitmap_.GetAddressOf()))) return;
    w_ = d.Width; h_ = d.Height;
}

void Hud::Draw(ID3D11Texture2D* target, const HudState& s) {
    EnsureBitmap(target);
    if (!bitmap_) return;

    ctx_->SetTarget(bitmap_.Get());
    ctx_->BeginDraw();

    // Translucent rounded pill, LS-style: two stat blocks (BASE / GENERATED)
    // plus a small summary line. Drawn every frame onto the presented frame.
    const D2D1_ROUNDED_RECT pill = { { 8.f, 8.f, 336.f, 106.f }, 12.f, 12.f };
    CPtr<ID2D1SolidColorBrush> bg;
    ctx_->CreateSolidColorBrush(D2D1::ColorF(0x0E1014, 0.66f), bg.GetAddressOf());
    ctx_->FillRoundedRectangle(pill, bg.Get());

    const D2D1_RECT_F rBaseLbl = { 26.f, 20.f, 140.f, 36.f };
    const D2D1_RECT_F rBaseVal = { 26.f, 36.f, 150.f, 76.f };
    const D2D1_RECT_F rGenLbl  = { 160.f, 20.f, 322.f, 36.f };
    const D2D1_RECT_F rGenVal  = { 160.f, 36.f, 322.f, 76.f };
    const D2D1_RECT_F rSub     = { 26.f, 76.f, 324.f, 96.f };

    ctx_->DrawTextW(L"BASE", 4, fmtTiny_.Get(), rBaseLbl, brushLabel_.Get());
    wchar_t num[32];
    swprintf_s(num, L"%.0f", double(s.srcFps));
    ctx_->DrawTextW(num, UINT32(wcslen(num)), fmtBig_.Get(), rBaseVal, brushWhite_.Get());

    ctx_->DrawTextW(L"GENERATED", 9, fmtTiny_.Get(), rGenLbl, brushLabel_.Get());
    if (s.genFps > 0.5f) {
        swprintf_s(num, L"+%.0f", double(s.genFps));
        ctx_->DrawTextW(num, UINT32(wcslen(num)), fmtBig_.Get(), rGenVal, brushAccent_.Get());
    } else {
        ctx_->DrawTextW(L"OFF", 3, fmtBig_.Get(), rGenVal, brushLabel_.Get());
    }

    wchar_t sub[160];
    swprintf_s(sub, L"out %.0f fps   %ls   p50 %.1f / p95 %.1f ms",
               double(s.outFps), s.fgLabel.c_str(), double(s.latencyP50), double(s.latencyP95));
    ctx_->DrawTextW(sub, UINT32(wcslen(sub)), fmtSmall_.Get(), rSub, brushLabel_.Get());

    ctx_->EndDraw();
    ctx_->SetTarget(nullptr);
}

void Hud::Resize(uint32_t width, uint32_t height, ID3D11Texture2D* backbuffer) {
    w_ = width; h_ = height;
    EnsureBitmap(backbuffer);
}

void Hud::Shutdown() {
    bitmap_.Reset(); ctx_.Reset(); d2dDev_.Reset(); factory_.Reset();
    fmtBig_.Reset(); fmtSmall_.Reset(); fmtTiny_.Reset(); dwrite_.Reset();
    brushWhite_.Reset(); brushAccent_.Reset(); brushLabel_.Reset();
}

} // namespace fs
