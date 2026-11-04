// dxgi1_4.h stub — IDXGISwapChain3 (color space), IDXGIFactory4, IDXGIAdapter3.
#pragma once
#include "dxgi1_3.h"

struct IDXGISwapChain3 : IDXGISwapChain2 {
    virtual HRESULT SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) { (void)ColorSpace; return E_NOTIMPL; }
    virtual HRESULT CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT* pColorSpaceSupport) {
        (void)ColorSpace; (void)pColorSpaceSupport; return E_NOTIMPL;
    }
};

struct IDXGIOutput3 : IDXGIOutput2 {
    virtual HRESULT CheckOverlaySupport(DXGI_FORMAT, IUnknown*, UINT*) { return E_NOTIMPL; }
};

struct IDXGIAdapter3 : IDXGIAdapter2 {
    virtual HRESULT RegisterVideoMemoryBudgetChangeNotificationEvent(HANDLE, DWORD*) { (void)0; return E_NOTIMPL; }
};

struct IDXGIFactory4 : IDXGIFactory3 {
    virtual HRESULT EnumAdapterByLuid(LUID, REFIID, void**) { return E_NOTIMPL; }
    virtual HRESULT EnumWarpAdapter(REFIID, void**) { return E_NOTIMPL; }
};

// ================= dxgi1_5 =================
#include "dxgi1_5.h"

FS_UUID(IDXGISwapChain3, 0x94d99bdb, 0xf1f8, 0x4ab0, 0xb2,0x36,0x7d,0xa0,0x17,0x0e,0xda,0xb1)
FS_UUID(IDXGIFactory4,   0x1bc6ea02, 0xef36, 0x464f, 0xbf,0x0c,0x21,0xca,0x39,0xe5,0x16,0x8a)
