// dxgi1_3.h stub — IDXGISwapChain2 (waitable object, frame latency).
#pragma once
#include "dxgi1_2.h"

typedef struct DXGI_MATRIX_3X2_F {
    FLOAT _11, _12, _21, _22, _31, _32;
} DXGI_MATRIX_3X2_F;

typedef enum DXGI_OFFER_RESOURCE_PRIORITY {
    DXGI_OFFER_RESOURCE_PRIORITY_LOW = 1,
    DXGI_OFFER_RESOURCE_PRIORITY_NORMAL = 2,
    DXGI_OFFER_RESOURCE_PRIORITY_HIGH = 3,
} DXGI_OFFER_RESOURCE_PRIORITY;

typedef struct DXGI_FRAME_STATISTICS {
    UINT PresentCount; UINT PresentRefreshCount; UINT SyncRefreshCount;
    LARGE_INTEGER SyncQPCTime; LARGE_INTEGER SyncGPUTime;
} DXGI_FRAME_STATISTICS;

struct IDXGISwapChain2 : IDXGISwapChain1 {
    virtual HRESULT SetSourceSize(UINT Width, UINT Height) { (void)Width; (void)Height; return E_NOTIMPL; }
    virtual HRESULT GetSourceSize(UINT*, UINT*) { return E_NOTIMPL; }
    virtual HRESULT SetMaximumFrameLatency(UINT MaxLatency) { (void)MaxLatency; return E_NOTIMPL; }
    virtual HRESULT GetMaximumFrameLatency(UINT* pLatency) { (void)pLatency; return E_NOTIMPL; }
    virtual HANDLE GetFrameLatencyWaitableObject() { return nullptr; }
    virtual HRESULT SetMatrixTransform(const DXGI_MATRIX_3X2_F*) { return E_NOTIMPL; }
    virtual HRESULT GetMatrixTransform(DXGI_MATRIX_3X2_F*) { return E_NOTIMPL; }
};

struct IDXGIOutput2 : IDXGIOutput {
    virtual BOOL SupportsOverlays() { return FALSE; }
};

struct IDXGIDevice2 : IDXGIDevice1 {
    virtual HRESULT OfferResources(UINT, void*, DXGI_OFFER_RESOURCE_PRIORITY) { return E_NOTIMPL; }
    virtual HRESULT ReclaimResources(UINT, void*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT EnqueueSetEvent(HANDLE) { return E_NOTIMPL; }
};

struct IDXGIFactory3 : IDXGIFactory2 {
    virtual UINT GetCreationFlags() { return 0; }
};

typedef struct DXGI_ADAPTER_DESC2 {
    WCHAR Description[128]; LUID AdapterLuid;
    size_t DedicatedVideoMemory; size_t DedicatedSystemMemory; size_t SharedSystemMemory;
    UINT Flags;
} DXGI_ADAPTER_DESC2;

struct IDXGIAdapter2 : IDXGIAdapter1 {
    virtual HRESULT GetDesc2(DXGI_ADAPTER_DESC2* pDesc) { (void)pDesc; return E_NOTIMPL; }
};

// ================= dxgi1_4 =================
#include "dxgi1_4.h"

FS_UUID(IDXGISwapChain2, 0x595e39a0, 0x0c60, 0x466b, 0x93,0x7f,0x3c,0xbc,0xef,0x07,0xf0,0x29)
FS_UUID(IDXGIFactory3,   0x25483823, 0xcd46, 0x4c7d, 0x86,0xa5,0xfe,0xcb,0xf3,0xdb,0xae,0x04)
