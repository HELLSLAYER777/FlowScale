// dwmapi.h stub
#pragma once
#include <windows.h>
enum DWMWINDOWATTRIBUTE {
    DWMWA_NCRENDERING_ENABLED = 1,
    DWMWA_CLOAKED = 14,
    DWMWA_USE_IMMERSIVE_DARK_MODE = 20,
};
enum DWM_CLOAK_TYPE { DWM_CLOAKED_APP = 1, DWM_CLOAKED_SHELL = 2 };
inline HRESULT DwmGetWindowAttribute(HWND hwnd, DWORD attr, void* pv, DWORD cb) {
    (void)hwnd; (void)attr; (void)pv; (void)cb; return E_NOTIMPL;
}
inline HRESULT DwmSetWindowAttribute(HWND hwnd, DWORD attr, const void* pv, DWORD cb) {
    (void)hwnd; (void)attr; (void)pv; (void)cb; return E_NOTIMPL;
}
inline HRESULT DwmFlush() { return E_NOTIMPL; }
