// dxgi.h stub — base DXGI (compile surface only, real signatures).
#pragma once
#include <windows.h>

typedef enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
    DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    DXGI_FORMAT_R16G16_FLOAT = 34,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
} DXGI_FORMAT;

typedef struct DXGI_RATIONAL { UINT Numerator; UINT Denominator; } DXGI_RATIONAL;
typedef struct DXGI_SAMPLE_DESC { UINT Count; UINT Quality; } DXGI_SAMPLE_DESC;

typedef enum DXGI_MODE_SCANLINE_ORDER {
    DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE = 0,
} DXGI_MODE_SCANLINE_ORDER;
typedef enum DXGI_MODE_SCALING { DXGI_MODE_SCALING_UNSPECIFIED = 0 } DXGI_MODE_SCALING;
typedef enum DXGI_MODE_ROTATION {
    DXGI_MODE_ROTATION_UNSPECIFIED = 0, DXGI_MODE_ROTATION_IDENTITY = 1,
} DXGI_MODE_ROTATION;

typedef struct DXGI_MODE_DESC {
    UINT Width; UINT Height; DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format; DXGI_MODE_SCANLINE_ORDER ScanlineOrdering; DXGI_MODE_SCALING Scaling;
} DXGI_MODE_DESC;

typedef enum DXGI_SWAP_EFFECT {
    DXGI_SWAP_EFFECT_DISCARD = 0, DXGI_SWAP_EFFECT_SEQUENTIAL = 1,
    DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL = 3, DXGI_SWAP_EFFECT_FLIP_DISCARD = 4,
} DXGI_SWAP_EFFECT;

typedef enum DXGI_SWAP_CHAIN_FLAG {
    DXGI_SWAP_CHAIN_FLAG_NONPREROTATED = 1,
    DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH = 2,
    DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE = 4,
    DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT = 0x400L,
} DXGI_SWAP_CHAIN_FLAG;

typedef enum DXGI_RESIDENCY { DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY = 2 } DXGI_RESIDENCY;

enum : HRESULT {
    DXGI_ERROR_INVALID_CALL   = 0x887A0001u,
    DXGI_ERROR_NOT_FOUND      = 0x887A0002u,
    DXGI_ERROR_ACCESS_LOST    = 0x887A0026u,
    DXGI_ERROR_WAIT_TIMEOUT   = 0x887A0027u,
    DXGI_ERROR_NOT_CURRENTLY_AVAILABLE = 0x887A002Au,
};

enum : UINT { DXGI_USAGE_SHADER_INPUT = 0x10L, DXGI_USAGE_RENDER_TARGET_OUTPUT = 0x40L,
              DXGI_USAGE_UNORDERED_ACCESS = 0x100L };
typedef UINT DXGI_USAGE;

typedef struct DXGI_SWAP_CHAIN_DESC {
    DXGI_MODE_DESC BufferDesc; DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage; UINT BufferCount; HWND OutputWindow; BOOL Windowed;
    DXGI_SWAP_EFFECT SwapEffect; UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

typedef struct DXGI_ADAPTER_DESC1 {
    WCHAR Description[128]; LUID AdapterLuid;
    size_t DedicatedVideoMemory; size_t DedicatedSystemMemory; size_t SharedSystemMemory;
    UINT Flags;
} DXGI_ADAPTER_DESC1;

typedef struct DXGI_OUTPUT_DESC {
    WCHAR DeviceName[32]; RECT DesktopCoordinates; BOOL AttachedToDesktop;
    DXGI_MODE_ROTATION Rotation; HMONITOR Monitor;
} DXGI_OUTPUT_DESC;

typedef struct DXGI_SURFACE_DESC {
    UINT Width; UINT Height; DXGI_FORMAT Format; DXGI_SAMPLE_DESC SampleDesc;
} DXGI_SURFACE_DESC;

// ----------------------------------------------------------- interfaces --
struct IDXGIObject : IUnknown {
    virtual HRESULT SetPrivateData(REFIID, UINT, const void*) { return E_NOTIMPL; }
    virtual HRESULT SetPrivateDataInterface(REFIID, const IUnknown*) { return E_NOTIMPL; }
    virtual HRESULT GetPrivateData(REFIID, UINT*, void*) { return E_NOTIMPL; }
    virtual HRESULT GetParent(REFIID riid, void** ppParent) { (void)riid; (void)ppParent; return E_NOTIMPL; }
};

struct IDXGIDeviceSubObject : IDXGIObject {
    virtual HRESULT GetDevice(REFIID riid, void** ppDevice) { (void)riid; (void)ppDevice; return E_NOTIMPL; }
};

struct IDXGIAdapter : IDXGIObject {
    virtual HRESULT EnumOutputs(UINT Output, struct IDXGIOutput** ppOutput) { (void)Output; (void)ppOutput; return DXGI_ERROR_NOT_FOUND; }
};
struct IDXGIAdapter1 : IDXGIAdapter {
    virtual HRESULT GetDesc1(DXGI_ADAPTER_DESC1* pDesc) { (void)pDesc; return E_NOTIMPL; }
};

struct IDXGISwapChain;
struct IDXGISurface;
struct IDXGIOutput : IDXGIObject {
    virtual HRESULT GetDesc(DXGI_OUTPUT_DESC* pDesc) { (void)pDesc; return E_NOTIMPL; }
};

struct IDXGIFactory : IDXGIObject {
    virtual HRESULT EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter) { (void)Adapter; (void)ppAdapter; return DXGI_ERROR_NOT_FOUND; }
    virtual HRESULT MakeWindowAssociation(HWND Window, UINT Flags) { (void)Window; (void)Flags; return E_NOTIMPL; }
    virtual HRESULT GetWindowAssociation(HWND* pWindow) { (void)pWindow; return E_NOTIMPL; }
    virtual HRESULT CreateSwapChain(IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**) { return E_NOTIMPL; }
    virtual HRESULT CreateSoftwareAdapter(HMODULE, IDXGIAdapter**) { return E_NOTIMPL; }
};

struct IDXGISwapChain : IDXGIDeviceSubObject {
    virtual HRESULT Present(UINT SyncInterval, UINT Flags) { (void)SyncInterval; (void)Flags; return E_NOTIMPL; }
    virtual HRESULT GetBuffer(UINT Buffer, REFIID riid, void** ppSurface) { (void)Buffer; (void)riid; (void)ppSurface; return E_NOTIMPL; }
    virtual HRESULT SetFullscreenState(BOOL, IDXGIOutput*) { return E_NOTIMPL; }
};

struct IDXGISurface : IDXGIDeviceSubObject {
    virtual HRESULT GetDesc(DXGI_SURFACE_DESC* pDesc) { (void)pDesc; return E_NOTIMPL; }
};

struct IDXGIResource : IDXGIDeviceSubObject {
    virtual HRESULT GetUsage(DXGI_USAGE* pUsage) { (void)pUsage; return E_NOTIMPL; }
};

struct IDXGIDevice : IDXGIObject {
    // NOTE: real base signature — takes IDXGIAdapter**, not IDXGIAdapter1**.
    virtual HRESULT GetAdapter(IDXGIAdapter** pAdapter) { (void)pAdapter; return E_NOTIMPL; }
};

// ================= dxgi1_2 =================
#include "dxgi1_2.h"

FS_UUID(IDXGIObject,      0x7922114d, 0x14db, 0x4a9b, 0x9b,0x8f,0xf0,0xc2,0x64,0xe8,0x9c,0x75)
FS_UUID(IDXGIDevice,      0x54ec77fa, 0x1377, 0x44e6, 0x8c,0x32,0x88,0xfd,0x5f,0x44,0xc8,0x4c)
FS_UUID(IDXGIAdapter,     0x2411e7e1, 0x12ac, 0x4ccf, 0xbd,0x14,0x97,0x98,0xe8,0x53,0x4d,0xc0)
FS_UUID(IDXGIAdapter1,    0x290462f0, 0xac2e, 0x4e41, 0xbe,0x59,0x96,0x84,0xd9,0x82,0x6f,0x0f)
FS_UUID(IDXGIOutput,      0x9d89fcb8, 0x5e44, 0x4ae2, 0x9e,0x97,0x5d,0xb8,0xd4,0x8e,0xd0,0x2c)
FS_UUID(IDXGISurface,     0xcafcb579, 0x6be3, 0x471a, 0xa2,0x5a,0x91,0xd2,0xc0,0x6c,0x4b,0x11)
FS_UUID(IDXGIResource,    0x035f3ab4, 0x482e, 0x4e50, 0xb4,0x1f,0x8a,0x7f,0x8b,0xd8,0x96,0x0b)
FS_UUID(IDXGISwapChain,   0x310d36a0, 0xd2e7, 0x4c0a, 0xaa,0x04,0x6a,0x9d,0x23,0xb8,0x88,0x6a)
FS_UUID(IDXGIFactory,     0xae02eedb, 0xc735, 0x4690, 0x8e,0x10,0x99,0x81,0x8c,0xb4,0xd7,0xd4)
