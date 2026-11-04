// winrt/Windows.Foundation.h stub
#pragma once
#include "base.h"

namespace winrt { namespace Windows { namespace Foundation {

struct IInspectable {
    IInspectable() noexcept = default;
    IInspectable(std::nullptr_t) noexcept {}
    IInspectable(void*, winrt::take_ownership_from_abi_t) noexcept {}
    IInspectable& operator=(std::nullptr_t) noexcept { return *this; }
    explicit operator bool() const noexcept { return false; }
    template <typename To> To as() const { return To{}; }
};

}}}
