// windows.graphics.directx.direct3d11.interop.h stub
#pragma once
#include <windows.h>
#include <unknwn.h>
#include <dxgi.h>

// ABI IDirect3DDevice (IInspectable-derived); the projected winrt type wraps it.
struct IDirect3DDevice : IInspectable {};

struct IDirect3DDxgiInterfaceAccess : IUnknown {
    virtual HRESULT GetInterface(REFIID iid, void** p) { (void)iid; (void)p; return E_NOTIMPL; }
};

extern "C" inline HRESULT CreateDirect3D11DeviceFromDXGIDevice(IDXGIDevice* dxgiDevice,
                                                               IInspectable** graphicsDevice) {
    (void)dxgiDevice; (void)graphicsDevice; return E_NOTIMPL;
}

FS_UUID(IDirect3DDevice,            0xa9e2c11c, 0x6f5a, 0x4a3f, 0xaf,0x62,0x27,0x49,0x69,0x9d,0x6e,0x6c)
FS_UUID(IDirect3DDxgiInterfaceAccess, 0xa9b94138, 0x3107, 0x4035, 0x9d,0x33,0x34,0x9e,0x58,0x35,0xd3,0x66)
