// dxgi1_5.h stub — enough for `#include <dxgi1_5.h>` (Presenter.cpp).
#pragma once
#include "dxgi1_4.h"

typedef enum DXGI_HDR_METADATA_TYPE {
    DXGI_HDR_METADATA_TYPE_NONE = 0,
    DXGI_HDR_METADATA_TYPE_HDR10 = 1,
} DXGI_HDR_METADATA_TYPE;

typedef enum DXGI_FEATURE {
    DXGI_FEATURE_PRESENT_ALLOW_TEARING = 0,
} DXGI_FEATURE;

typedef struct DXGI_HDR_METADATA_HDR10 {
    UINT16 RedPrimary[2]; UINT16 GreenPrimary[2]; UINT16 BluePrimary[2];
    UINT16 WhitePoint[2]; UINT MaxMasteringLuminance; UINT MinMasteringLuminance;
    UINT16 MaxContentLightLevel; UINT16 MaxFrameAverageLightLevel;
} DXGI_HDR_METADATA_HDR10;

struct IDXGIOutput4 : IDXGIOutput3 {
    virtual HRESULT CheckOverlayColorSpaceSupport(DXGI_FORMAT, DXGI_COLOR_SPACE_TYPE, IUnknown*, UINT*) { return E_NOTIMPL; }
};

struct IDXGISwapChain4 : IDXGISwapChain3 {
    virtual HRESULT SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void* pData) { (void)Type; (void)Size; (void)pData; return E_NOTIMPL; }
};

struct IDXGIAdapter4 : IDXGIAdapter3 {};
struct IDXGIFactory5 : IDXGIFactory4 {
    virtual HRESULT CheckFeatureSupport(DXGI_FEATURE Feature, void* pFeatureSupport, UINT FeatureSupportSize) {
        (void)Feature; (void)pFeatureSupport; (void)FeatureSupportSize; return E_NOTIMPL;
    }
};
