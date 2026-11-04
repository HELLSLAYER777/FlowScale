// dwrite.h stub — compile surface only.
#pragma once
#include <windows.h>

typedef enum DWRITE_FACTORY_TYPE {
    DWRITE_FACTORY_TYPE_SHARED = 0, DWRITE_FACTORY_TYPE_ISOLATED = 1,
} DWRITE_FACTORY_TYPE;

typedef enum DWRITE_FONT_WEIGHT {
    DWRITE_FONT_WEIGHT_NORMAL = 400, DWRITE_FONT_WEIGHT_SEMI_BOLD = 600, DWRITE_FONT_WEIGHT_BOLD = 700,
} DWRITE_FONT_WEIGHT;

typedef enum DWRITE_FONT_STYLE {
    DWRITE_FONT_STYLE_NORMAL = 0, DWRITE_FONT_STYLE_OBLIQUE = 1, DWRITE_FONT_STYLE_ITALIC = 2,
} DWRITE_FONT_STYLE;

typedef enum DWRITE_FONT_STRETCH {
    DWRITE_FONT_STRETCH_NORMAL = 5, DWRITE_FONT_STRETCH_CONDENSED = 4,
} DWRITE_FONT_STRETCH;

typedef enum DWRITE_TEXT_ALIGNMENT {
    DWRITE_TEXT_ALIGNMENT_LEADING = 0, DWRITE_TEXT_ALIGNMENT_CENTER = 1, DWRITE_TEXT_ALIGNMENT_TRAILING = 2,
} DWRITE_TEXT_ALIGNMENT;

typedef enum DWRITE_PARAGRAPH_ALIGNMENT {
    DWRITE_PARAGRAPH_ALIGNMENT_NEAR = 0, DWRITE_PARAGRAPH_ALIGNMENT_FAR = 1,
    DWRITE_PARAGRAPH_ALIGNMENT_CENTER = 2, DWRITE_PARAGRAPH_ALIGNMENT_JUSTIFIED = 3,
} DWRITE_PARAGRAPH_ALIGNMENT;

struct IDWriteTextFormat : IUnknown {
    virtual HRESULT SetTextAlignment(DWRITE_TEXT_ALIGNMENT textAlignment) { (void)textAlignment; return S_OK; }
    virtual HRESULT SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment) {
        (void)paragraphAlignment; return S_OK;
    }
};
struct IDWriteFontCollection : IUnknown {};

struct IDWriteFactory : IUnknown {
    virtual HRESULT CreateTextFormat(const WCHAR* localeCode, IDWriteFontCollection* fontCollection,
                                     DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle,
                                     DWRITE_FONT_STRETCH fontStretch, FLOAT fontSize,
                                     const WCHAR* localeName, IDWriteTextFormat** textFormat) {
        (void)localeCode; (void)fontCollection; (void)fontWeight; (void)fontStyle; (void)fontStretch;
        (void)fontSize; (void)localeName; (void)textFormat; return E_NOTIMPL;
    }
};

inline HRESULT DWriteCreateFactory(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown** factory) {
    (void)factoryType; (void)iid; (void)factory; return E_NOTIMPL;
}

FS_UUID(IDWriteFactory,    0xb859ee5a, 0xd838, 0x4b5b, 0xa2,0xe8,0x1a,0xdc,0x7d,0x93,0xdb,0x48)
FS_UUID(IDWriteTextFormat, 0x9c906818, 0x31d7, 0x4fd3, 0xa2,0x02,0xe3,0x2a,0x2e,0x33,0x6a,0xe9)
