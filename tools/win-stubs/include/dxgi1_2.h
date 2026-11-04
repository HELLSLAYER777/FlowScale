// dxgi1_2.h stub — IDXGIFactory2 / IDXGISwapChain1 / Desktop Duplication.
#pragma once
#include "dxgi.h"

typedef enum DXGI_SCALING {
    DXGI_SCALING_STRETCH = 0, DXGI_SCALING_NONE = 1, DXGI_SCALING_ASPECT_RATIO_STRETCH = 2,
} DXGI_SCALING;

typedef enum DXGI_ALPHA_MODE {
    DXGI_ALPHA_MODE_UNSPECIFIED = 0, DXGI_ALPHA_MODE_IGNORE = 3,
} DXGI_ALPHA_MODE;

typedef enum DXGI_COLOR_SPACE_TYPE {
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709 = 0,
    DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 = 12,
} DXGI_COLOR_SPACE_TYPE;

enum : UINT {
    DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT = 0x1,
    DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_OVERLAY = 0x2,
};

enum : UINT {
    DXGI_MWA_NO_WINDOW_CHANGES = 0x1L,
    DXGI_MWA_NO_ALT_ENTER = 0x2L,
};

typedef struct DXGI_SWAP_CHAIN_DESC1 {
    UINT Width; UINT Height; DXGI_FORMAT Format; BOOL Stereo;
    DXGI_SAMPLE_DESC SampleDesc; DXGI_USAGE BufferUsage; UINT BufferCount;
    DXGI_SCALING Scaling; DXGI_SWAP_EFFECT SwapEffect; DXGI_ALPHA_MODE AlphaMode; UINT Flags;
} DXGI_SWAP_CHAIN_DESC1;

typedef struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC {
    DXGI_RATIONAL RefreshRate; DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling; BOOL Windowed;
} DXGI_SWAP_CHAIN_FULLSCREEN_DESC;

typedef struct DXGI_OUTDUPL_DESC {
    DXGI_MODE_DESC ModeDesc; DXGI_MODE_ROTATION Rotation; BOOL DesktopImageInSystemMemory;
} DXGI_OUTDUPL_DESC;

typedef struct DXGI_OUTDUPL_FRAME_INFO {
    LARGE_INTEGER LastPresentTime; LARGE_INTEGER LastMouseUpdateTime;
    UINT AccumulatedFrames; BOOL RectsCoalesced; BOOL ProtectedContentMaskedOut;
    RECT LastPresent; RECT LastMouseVisible;
    UINT TotalMetadataBufferSize; UINT PointerShapeBufferSize;
} DXGI_OUTDUPL_FRAME_INFO;

struct IDXGIFactory1 : IDXGIFactory {
    virtual HRESULT EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter) { (void)Adapter; (void)ppAdapter; return DXGI_ERROR_NOT_FOUND; }
    virtual BOOL IsCurrent() { return TRUE; }
};

struct IDXGIDevice1 : IDXGIDevice {
    virtual HRESULT SetMaximumFrameLatency(UINT MaxLatency) { (void)MaxLatency; return E_NOTIMPL; }
    virtual HRESULT GetMaximumFrameLatency(UINT* pLatency) { (void)pLatency; return E_NOTIMPL; }
};

struct IDXGISurface2 : IDXGISurface {};

struct IDXGIResource1 : IDXGIResource {};

struct IDXGISwapChain1 : IDXGISwapChain {
    virtual HRESULT GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc) { (void)pDesc; return E_NOTIMPL; }
    virtual HRESULT GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc) { (void)pDesc; return E_NOTIMPL; }
    virtual HRESULT GetHwnd(HWND* pHwnd) { (void)pHwnd; return E_NOTIMPL; }
    virtual HRESULT Present1(UINT SyncInterval, UINT PresentFlags, const void* pPresentParameters) { (void)SyncInterval; (void)PresentFlags; (void)pPresentParameters; return E_NOTIMPL; }
    virtual BOOL IsTemporaryMonoSupported() { return FALSE; }
    virtual HRESULT GetRestrictToOutput(IDXGIOutput**) { return E_NOTIMPL; }
    virtual HRESULT SetRotation(DXGI_MODE_ROTATION) { return E_NOTIMPL; }
    virtual HRESULT GetRotation(DXGI_MODE_ROTATION*) { return E_NOTIMPL; }
};

struct IDXGIOutputDuplication : IDXGIObject {
    virtual HRESULT GetDesc(DXGI_OUTDUPL_DESC* pDesc) { (void)pDesc; return E_NOTIMPL; }
    virtual HRESULT AcquireNextFrame(UINT TimeoutInMilliseconds, DXGI_OUTDUPL_FRAME_INFO* pFrameInfo, IDXGIResource** ppDesktopResource) {
        (void)TimeoutInMilliseconds; (void)pFrameInfo; (void)ppDesktopResource; return E_NOTIMPL;
    }
    virtual HRESULT ReleaseFrame() { return E_NOTIMPL; }
};

struct IDXGIOutput1 : IDXGIOutput {
    virtual HRESULT GetDisplaySurfaceData1(IDXGIResource*) { return E_NOTIMPL; }
    virtual HRESULT DuplicateOutput(IUnknown* pDevice, IDXGIOutputDuplication** ppOutputDuplication) {
        (void)pDevice; (void)ppOutputDuplication; return E_NOTIMPL;
    }
};

struct IDXGIFactory2 : IDXGIFactory1 {
    virtual BOOL IsWindowedStereoEnabled() { return FALSE; }
    virtual HRESULT CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc,
                                           const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
                                           IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
        (void)pDevice; (void)hWnd; (void)pDesc; (void)pFullscreenDesc; (void)pRestrictToOutput; (void)ppSwapChain;
        return E_NOTIMPL;
    }
    virtual HRESULT CreateSwapChainForCoreWindow(IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*,
                                                 IDXGIOutput*, IDXGISwapChain1**) { return E_NOTIMPL; }
    virtual HRESULT CreateSwapChainForComposition(IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**) { return E_NOTIMPL; }
};

// ================= dxgi1_3 =================
#include "dxgi1_3.h"

FS_UUID(IDXGIFactory1,      0x770aae78, 0xf26c, 0x4db6, 0xa6,0x29,0x0c,0x51,0x5e,0xdc,0x8b,0xb9)
FS_UUID(IDXGIFactory2,      0x50c83a1c, 0xe072, 0x4c48, 0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0)
FS_UUID(IDXGISwapChain1,    0x790a45f7, 0x0d42, 0x4876, 0x98,0x3a,0x0a,0x55,0xcf,0xe6,0xf4,0xaa)
FS_UUID(IDXGIOutput1,       0x00cddea8, 0x939b, 0x4b83, 0xa3,0x40,0xae,0x0e,0xd4,0x1a,0xd7,0xc5)
FS_UUID(IDXGIOutputDuplication, 0x191e8b02, 0xc523, 0x4bf9, 0x8e,0xbe,0xfc,0x0f,0x2a,0x62,0x14,0x76)
FS_UUID(IDXGIDevice1,       0x77db970f, 0x6276, 0x48ba, 0xba,0x28,0x07,0x01,0x43,0xb4,0x39,0x2c)
