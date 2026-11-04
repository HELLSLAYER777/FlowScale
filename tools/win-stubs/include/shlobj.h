// shlobj.h stub
#pragma once
#include <windows.h>
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c
#endif
inline HRESULT SHGetFolderPathW(HWND hwnd, int csidl, HANDLE token, DWORD flags, LPWSTR path) {
    (void)hwnd; (void)csidl; (void)token; (void)flags; if (path) path[0] = 0; return E_NOTIMPL;
}
