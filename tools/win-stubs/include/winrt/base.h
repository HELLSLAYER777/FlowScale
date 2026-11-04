// winrt/base.h stub — minimal C++/WinRT core used by FlowScale.
// Mirrors the compile-time surface (names, arities) of the real cppwinrt base.
#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

namespace winrt {

// ---- hstring ----
struct hstring {
    hstring() noexcept = default;
    hstring(std::nullptr_t) {}
    hstring(wchar_t const* s) : v_(s ? s : L"") {}
    hstring(std::wstring const& s) : v_(s) {}
    wchar_t const* c_str() const noexcept { return v_.c_str(); }
    bool empty() const noexcept { return v_.empty(); }
    uint32_t size() const noexcept { return static_cast<uint32_t>(v_.size()); }
    std::wstring const& std_string() const { return v_; }
private:
    std::wstring v_;
};

inline bool operator==(hstring const& a, hstring const& b) { return a.std_string() == b.std_string(); }
inline bool operator<(hstring const& a, hstring const& b) { return a.std_string() < b.std_string(); }

// ---- errors ----
struct hresult_error : std::exception {
    hresult_error() = default;
    explicit hresult_error(hstring const& message) : msg_(message) {}
    hstring message() const { return msg_; }
    int32_t code() const noexcept { return 0x80004005; }
    char const* what() const noexcept override { return "winrt hresult_error"; }
private:
    hstring msg_;
};

// ---- guid ----
struct guid {
    GUID value{};
    constexpr operator GUID() const noexcept { return value; }
};

template <typename T> constexpr guid guid_of() noexcept;

// ---- com_ptr (raw ABI interfaces) ----
template <typename T>
struct com_ptr {
    com_ptr() noexcept = default;
    com_ptr(std::nullptr_t) noexcept {}
    T** put() noexcept { return &ptr_; }
    T* get() const noexcept { return ptr_; }
    T** operator&() noexcept { return &ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    T* operator->() const noexcept { return ptr_; }
    template <typename To> To as() const { return To{}; }
private:
    T* ptr_ = nullptr;
};

// ---- agile_ref ----
template <typename T>
struct agile_ref {
    agile_ref() noexcept = default;
    agile_ref(std::nullptr_t) noexcept {}
    agile_ref(T const& object) : obj_(object) {}
    agile_ref& operator=(T const& object) { obj_ = object; return *this; }
    agile_ref& operator=(std::nullptr_t) { obj_ = T{ nullptr }; return *this; }
    T get() const { return obj_; }
    operator T() const { return obj_; }
    explicit operator bool() const noexcept { return static_cast<bool>(obj_); }
private:
    T obj_{ nullptr };
};

// ---- put_abi / get_abi (projected types: writes void**) ----
template <typename T> void** put_abi(T& object) noexcept {
    (void)object;
    return reinterpret_cast<void**>(&object);
}
template <typename T> void* get_abi(T const& object) noexcept {
    (void)object;
    return nullptr;
}

// ---- event tokens / auto_revoke ----
struct event_token { int64_t value = 0; bool operator==(event_token const&) const = default; };
struct auto_revoke_t {};
inline constexpr auto_revoke_t auto_revoke{};

// ---- param-point helpers used in interop ----
struct take_ownership_from_abi_t {};
inline constexpr take_ownership_from_abi_t take_ownership_from_abi{};

} // namespace winrt
