// d2d1.h / d2d1_1.h / d2d1helper.h stub — compile surface only.
#pragma once
#include <windows.h>
#include <dxgi.h>

typedef enum D2D1_ALPHA_MODE {
    D2D1_ALPHA_MODE_UNKNOWN = 0, D2D1_ALPHA_MODE_PREMULTIPLIED = 1, D2D1_ALPHA_MODE_IGNORE = 3,
} D2D1_ALPHA_MODE;

typedef enum D2D1_FACTORY_TYPE {
    D2D1_FACTORY_TYPE_SINGLE_THREADED = 0, D2D1_FACTORY_TYPE_MULTI_THREADED = 1,
} D2D1_FACTORY_TYPE;

typedef enum D2D1_BITMAP_OPTIONS {
    D2D1_BITMAP_OPTIONS_NONE = 0,
    D2D1_BITMAP_OPTIONS_TARGET = 0x1,
    D2D1_BITMAP_OPTIONS_CANNOT_DRAW = 0x2,
} D2D1_BITMAP_OPTIONS;

// real SDK defines these flag operators via DEFINE_ENUM_FLAG_OPERATORS
inline D2D1_BITMAP_OPTIONS operator|(D2D1_BITMAP_OPTIONS a, D2D1_BITMAP_OPTIONS b) {
    return static_cast<D2D1_BITMAP_OPTIONS>(static_cast<UINT>(a) | static_cast<UINT>(b));
}

typedef enum D2D1_DEVICE_CONTEXT_OPTIONS {
    D2D1_DEVICE_CONTEXT_OPTIONS_NONE = 0,
} D2D1_DEVICE_CONTEXT_OPTIONS;

typedef enum D2D1_PRESENT_OPTIONS { D2D1_PRESENT_OPTIONS_NONE = 0 } D2D1_PRESENT_OPTIONS;
typedef enum D2D1_ANTIALIAS_MODE { D2D1_ANTIALIAS_MODE_PER_PRIMITIVE = 0 } D2D1_ANTIALIAS_MODE;
typedef enum D2D1_TEXT_ANTIALIAS_MODE { D2D1_TEXT_ANTIALIAS_MODE_DEFAULT = 0 } D2D1_TEXT_ANTIALIAS_MODE;

typedef struct D2D1_PIXEL_FORMAT {
    DXGI_FORMAT format; D2D1_ALPHA_MODE alphaMode;
} D2D1_PIXEL_FORMAT;

typedef struct D2D1_POINT_2F { FLOAT x; FLOAT y; } D2D1_POINT_2F;
typedef struct D2D1_SIZE_F { FLOAT width; FLOAT height; } D2D1_SIZE_F;
typedef struct D2D1_RECT_F { FLOAT left; FLOAT top; FLOAT right; FLOAT bottom; } D2D1_RECT_F;
typedef struct D2D1_COLOR_F { FLOAT r; FLOAT g; FLOAT b; FLOAT a; } D2D1_COLOR_F;

typedef struct D2D1_ROUNDED_RECT {
    D2D1_RECT_F rect; FLOAT radiusX; FLOAT radiusY;
} D2D1_ROUNDED_RECT;

typedef struct D2D1_BITMAP_PROPERTIES {
    D2D1_PIXEL_FORMAT pixelFormat; FLOAT dpiX; FLOAT dpiY;
} D2D1_BITMAP_PROPERTIES;

typedef struct D2D1_BITMAP_PROPERTIES1 {
    D2D1_BITMAP_OPTIONS bitmapOptions; D2D1_PIXEL_FORMAT pixelFormat; FLOAT dpiX; FLOAT dpiY;
    struct ID2D1ColorContext* colorContext;
} D2D1_BITMAP_PROPERTIES1;

// ------------------------------------------------------------ interfaces --
struct ID2D1Resource : IUnknown {};
struct ID2D1Image : ID2D1Resource {};
struct ID2D1ColorContext : ID2D1Resource {};
struct ID2D1Bitmap : ID2D1Image {};
struct ID2D1Bitmap1 : ID2D1Bitmap {};
struct ID2D1Brush : ID2D1Resource {};
struct ID2D1SolidColorBrush : ID2D1Brush {};
struct ID2D1RenderTarget : IUnknown {
    virtual HRESULT CreateSolidColorBrush(const D2D1_COLOR_F& color, ID2D1SolidColorBrush** brush) {
        (void)color; (void)brush; return E_NOTIMPL;
    }
    virtual void DrawTextW(const WCHAR* string, UINT32 stringLength, struct IDWriteTextFormat* textFormat,
                           const D2D1_RECT_F& layoutRect, ID2D1Brush* foregroundBrush,
                           D2D1_TEXT_ANTIALIAS_MODE = D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
                           D2D1_ANTIALIAS_MODE = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE) {
        (void)string; (void)stringLength; (void)textFormat; (void)layoutRect; (void)foregroundBrush;
    }
    virtual void FillRoundedRectangle(const D2D1_ROUNDED_RECT& rect, ID2D1Brush* brush) { (void)rect; (void)brush; }
    virtual void BeginDraw() {}
    virtual HRESULT EndDraw(UINT64* tag1 = nullptr, UINT64* tag2 = nullptr) { (void)tag1; (void)tag2; return E_NOTIMPL; }
    virtual D2D1_SIZE_F GetSize() const { return D2D1_SIZE_F{}; }
    virtual void SetTarget(ID2D1Image* target) { (void)target; }
};
struct ID2D1Factory : IUnknown {
    virtual HRESULT CreateDevice(struct IDXGIDevice* dxgiDevice, struct ID2D1Device** d2dDevice) {
        (void)dxgiDevice; (void)d2dDevice; return E_NOTIMPL;
    }
};
// Factory1 is what Hud stores (needed for CreateDevice on a DXGI device and
// the D2D 1.1 API surface). API usage beyond the inherited CreateDevice is
// not exercised by FlowScale, so the base signature suffices.
struct ID2D1Factory1 : ID2D1Factory {};
struct ID2D1Device : IUnknown {
    virtual HRESULT CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS options, struct ID2D1DeviceContext** deviceContext) {
        (void)options; (void)deviceContext; return E_NOTIMPL;
    }
};
struct ID2D1DeviceContext : ID2D1RenderTarget {
    virtual HRESULT CreateBitmapFromDxgiSurface(IDXGISurface* surface, const D2D1_BITMAP_PROPERTIES1* bitmapProperties,
                                                ID2D1Bitmap1** bitmap) {
        (void)surface; (void)bitmapProperties; (void)bitmap; return E_NOTIMPL;
    }
};

inline HRESULT D2D1CreateFactory(D2D1_FACTORY_TYPE factoryType, REFIID riid,
                                 const void* pFactoryOptions, void** ppIFactory) {
    (void)factoryType; (void)riid; (void)pFactoryOptions; (void)ppIFactory; return E_NOTIMPL;
}
template <class Factory>
inline HRESULT D2D1CreateFactory(D2D1_FACTORY_TYPE factoryType, Factory** factory) {
    return D2D1CreateFactory(factoryType, FsUuid<Factory>::value, nullptr,
                             reinterpret_cast<void**>(factory));
}

// d2d1helper.h namespace
namespace D2D1 {

struct ColorF {
    enum CommonColor : UINT32 {};
    ColorF(UINT32 rgb, FLOAT alpha = 1.0f)
        : a(alpha), r(static_cast<FLOAT>((rgb & 0x00FF0000) >> 16) / 255.0f),
          g(static_cast<FLOAT>((rgb & 0x0000FF00) >> 8) / 255.0f),
          b(static_cast<FLOAT>(rgb & 0x000000FF) / 255.0f) {}
    FLOAT a, r, g, b;
    operator D2D1_COLOR_F() const { return D2D1_COLOR_F{ r, g, b, a }; }
};

inline D2D1_PIXEL_FORMAT PixelFormat(DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN,
                                     D2D1_ALPHA_MODE alphaMode = D2D1_ALPHA_MODE_UNKNOWN) {
    D2D1_PIXEL_FORMAT pf; pf.format = dxgiFormat; pf.alphaMode = alphaMode; return pf;
}

inline D2D1_BITMAP_PROPERTIES1 BitmapProperties1(D2D1_BITMAP_OPTIONS bitmapOptions = D2D1_BITMAP_OPTIONS_NONE,
                                                 const D2D1_PIXEL_FORMAT& pixelFormat = PixelFormat(),
                                                 FLOAT dpiX = 96.0f, FLOAT dpiY = 96.0f) {
    D2D1_BITMAP_PROPERTIES1 bp; bp.bitmapOptions = bitmapOptions; bp.pixelFormat = pixelFormat;
    bp.dpiX = dpiX; bp.dpiY = dpiY; bp.colorContext = nullptr; return bp;
}

} // namespace D2D1

FS_UUID(ID2D1Factory,        0x06152247, 0x6f50, 0x465a, 0x92,0x45,0x11,0x8b,0xfd,0x3b,0x60,0x07)
FS_UUID(ID2D1Factory1,       0xbb12d362, 0xdaee, 0x4b9a, 0xaa,0x9d,0x14,0xba,0x0a,0xcc,0xc2,0xfa)
FS_UUID(ID2D1Device,         0x47dd524d, 0xaa9d, 0x4b45, 0x8b,0x00,0x62,0x8f,0xd1,0xf2,0x0d,0x8b)
FS_UUID(ID2D1DeviceContext,  0xe8f7fe7a, 0x191c, 0x4669, 0x9a,0x2b,0xf2,0xe0,0xd6,0xd1,0x17,0xea)
FS_UUID(ID2D1Bitmap1,        0xa8984682, 0x4c9f, 0x4fed, 0x82,0xe0,0x05,0x4f,0x3b,0x0d,0x5b,0x9e)
FS_UUID(ID2D1SolidColorBrush,0x2b3e6fa8, 0x7a15, 0x4959, 0xa4,0xc6,0x04,0x8f,0x93,0xd2,0x1e,0x94)
