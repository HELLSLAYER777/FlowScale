// wrl/wrappers/corewrappers.h stub — WRL-scoped HStringReference used by
// WGCCapture.cpp for RoGetActivationFactory (mirrors the real header layout:
// Microsoft::WRL::Wrappers::HStringReference, distinct from winstring.h's
// global-namespace helper).
#pragma once
#include <windows.h>
#include <string>
#include <winstring.h>

namespace Microsoft { namespace WRL { namespace Wrappers {

class HStringReference {
public:
    explicit HStringReference(const wchar_t* s) : s_(s ? s : L"") {}
    HSTRING Get() const { return nullptr; }   // value irrelevant for syntax check
private:
    std::wstring s_;
};

}}} // namespace Microsoft::WRL::Wrappers
