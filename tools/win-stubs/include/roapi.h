// roapi.h stub
#pragma once
#include <windows.h>
#include <winstring.h>
inline HRESULT RoGetActivationFactory(HSTRING activatableClassId, REFIID iid, void** ppFactory) {
    (void)activatableClassId; (void)iid; (void)ppFactory; return E_NOTIMPL;
}
inline HRESULT RoInitialize(int) { return E_NOTIMPL; }
inline void RoUninitialize() {}
inline HRESULT CoInitializeEx(void*, DWORD) { return S_OK; }
inline void CoUninitialize() {}
