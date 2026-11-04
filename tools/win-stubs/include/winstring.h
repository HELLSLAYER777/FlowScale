// winstring.h stub — HSTRING basics.
#pragma once
#include <windows.h>
class HStringReference {
public:
    explicit HStringReference(const wchar_t* s) : s_(s ? s : L"") {}
    HSTRING Get() const { return nullptr; }   // value irrelevant for syntax check
private:
    std::wstring s_;
};
inline HRESULT WindowsCreateString(const wchar_t*, UINT32, HSTRING*) { return E_NOTIMPL; }
inline HRESULT WindowsDeleteString(HSTRING) { return E_NOTIMPL; }
