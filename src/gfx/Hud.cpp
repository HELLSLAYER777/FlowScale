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
                 DWRITE_FONT_STRETCH_NORMAL, 22.0f, L"en-us", fmtBig_.GetAddressOf()), "TextFormat big");
    ThrowIfFailed(dwrite_->CreateTextFormat(L"Consolas", nullptr,
                 DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                 DWRITE_FONT_STRETCH_NORMAL, 17.0f, L"en-us", fmtSmall_.GetAddressOf()), "TextFormat small");

    ThrowIfFailed(ctx_->CreateSolidColorBrush(D2D1::ColorF(0xF2F2F2, 1.0f), brushWhite_.GetAddressOf()), "Brush");
    ThrowIfFailed(ctx_->CreateSolidColorBrush(D2D1::ColorF(0x4FC3F7, 1.0f), brushAccent_.GetAddressOf()), "Brush accent");

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

    // Backdrop strip for legibility.
    const D2D1_SIZE_F sz = ctx_->GetSize();
    D2D1_ROUNDED_RECT strip = { { 8.f, 8.f, std::min(sz.width, 330.f), std::min(sz.height, 150.f) }, 8.f, 8.f };
    CPtr<ID2D1SolidColorBrush> bg;
    ctx_->CreateSolidColorBrush(D2D1::ColorF(0x101418, 0.62f), bg.GetAddressOf());
    ctx_->FillRoundedRectangle(strip, bg.Get());

    wchar_t line1[128], line2[128], line3[128];
    swprintf_s(line1, L"FlowScale  in %.0f fps   out %.0f fps", double(s.srcFps), double(s.outFps));
    swprintf_s(line2, L"FG %ls (%.0f fps gen)%ls", s.fgLabel.c_str(), double(s.genFps),
               s.neuralActive ? L"  [neural]" : L"");
    swprintf_s(line3, L"latency p50 %.1f ms   p95 %.1f ms", double(s.latencyP50), double(s.latencyP95));

    const D2D1_POINT_2F org = { 22.f, 18.f };
    ctx_->DrawTextW(line1, UINT32(wcslen(line1)), fmtBig_.Get(),   { org.x, org.y, org.x + 300, org.y + 34 }, brushWhite_.Get());
    ctx_->DrawTextW(line2, UINT32(wcslen(line2)), fmtSmall_.Get(), { org.x, org.y + 40, org.x + 300, org.y + 66 }, brushAccent_.Get());
    ctx_->DrawTextW(line3, UINT32(wcslen(line3)), fmtSmall_.Get(), { org.x, org.y + 68, org.x + 300, org.y + 94 }, brushWhite_.Get());
    if (!s.algoLabel.empty()) {
        ctx_->DrawTextW(s.algoLabel.c_str(), UINT32(s.algoLabel.size()), fmtSmall_.Get(),
                        { org.x, org.y + 96, org.x + 300, org.y + 122 }, brushWhite_.Get());
    }

    ctx_->EndDraw();
    ctx_->SetTarget(nullptr);
}

void Hud::Resize(uint32_t width, uint32_t height, ID3D11Texture2D* backbuffer) {
    w_ = width; h_ = height;
    EnsureBitmap(backbuffer);
}

void Hud::Shutdown() {
    bitmap_.Reset(); ctx_.Reset(); d2dDev_.Reset(); factory_.Reset();
    fmtBig_.Reset(); fmtSmall_.Reset(); dwrite_.Reset();
    brushWhite_.Reset(); brushAccent_.Reset();
}

} // namespace fs
