// windows.graphics.capture.interop.h stub — IGraphicsCaptureItemInterop.
#pragma once
#include <windows.h>
#include <unknwn.h>

struct IGraphicsCaptureItemInterop : IUnknown {
    virtual HRESULT CreateForWindow(HWND window, REFIID riid, void** result) {
        (void)window; (void)riid; (void)result; return E_NOTIMPL;
    }
    virtual HRESULT CreateForMonitor(HMONITOR hmon, REFIID riid, void** result) {
        (void)hmon; (void)riid; (void)result; return E_NOTIMPL;
    }
};

FS_UUID(IGraphicsCaptureItemInterop,
        0x3628e81b, 0x3cac, 0x4c60, 0xb7,0xf4,0x23,0xce,0x0e,0x0c,0x33,0x56)
