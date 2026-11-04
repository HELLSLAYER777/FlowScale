// wrl/client.h stub — Microsoft::WRL::ComPtr with the real member surface.
#pragma once
#include <windows.h>
#include <type_traits>

namespace Microsoft {
namespace WRL {

template <typename T>
class ComPtr {
public:
    ComPtr() noexcept : ptr_(nullptr) {}
    ComPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}
    ComPtr(T* other) noexcept : ptr_(other) { if (ptr_) ptr_->AddRef(); }
    template <class U> ComPtr(U* other) noexcept : ptr_(other) { if (ptr_) ptr_->AddRef(); }
    ComPtr(const ComPtr& other) noexcept : ptr_(other.ptr_) { if (ptr_) ptr_->AddRef(); }
    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    ComPtr& operator=(const ComPtr& other) noexcept {
        if (this != &other) InternalCopy(other.ptr_);
        return *this;
    }
    ComPtr& operator=(T* other) noexcept { InternalCopy(other); return *this; }
    ComPtr& operator=(std::nullptr_t) noexcept { InternalRelease(); return *this; }
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) { InternalRelease(); ptr_ = other.ptr_; other.ptr_ = nullptr; }
        return *this;
    }

    ~ComPtr() noexcept { InternalRelease(); }

    T* Get() const noexcept { return ptr_; }
    T** GetAddressOf() noexcept { return &ptr_; }
    T** ReleaseAndGetAddressOf() noexcept { InternalRelease(); return &ptr_; }
    T* Detach() noexcept { T* p = ptr_; ptr_ = nullptr; return p; }
    void Attach(T* p) noexcept { if (ptr_) ptr_->Release(); ptr_ = p; }
    void Reset() noexcept { InternalRelease(); }

    T* operator->() const noexcept { return ptr_; }
    operator T*() const noexcept { return ptr_; }   // real WRL: implicit raw-pointer conversion
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // Real WRL surface: As / AsIID / CopyTo (ComPtr is NOT an IUnknown —
    // it has no QueryInterface member; that is ATL's CComPtr).
    template <class U>
    HRESULT As(ComPtr<U>* p) const noexcept {
        if (!p) return E_POINTER;
        if (!ptr_) { p->Reset(); return S_OK; }
        return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
    }
    HRESULT AsIID(REFIID riid, ComPtr* p) const noexcept {
        if (!p) return E_POINTER;
        if (!ptr_) { p->Reset(); return S_OK; }
        return ptr_->QueryInterface(riid, reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
    }
    template <class U>
    HRESULT CopyTo(U** pp) const noexcept {
        if (!pp) return E_POINTER;
        if (!ptr_) { *pp = nullptr; return S_OK; }
        return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void**>(pp));
    }
    HRESULT CopyTo(T** pp) const noexcept {
        if (!pp) return E_POINTER;
        if (!ptr_) { *pp = nullptr; return S_OK; }
        *pp = ptr_; ptr_->AddRef(); return S_OK;
    }

private:
    void InternalCopy(T* other) noexcept {
        if (other) other->AddRef();
        InternalRelease();
        ptr_ = other;
    }
    void InternalRelease() noexcept {
        if (ptr_) { T* p = ptr_; ptr_ = nullptr; p->Release(); }
    }
    T* ptr_ = nullptr;

    // nullptr comparisons (the real WRL provides these)
    friend bool operator==(const ComPtr& a, std::nullptr_t) noexcept { return a.ptr_ == nullptr; }
    friend bool operator!=(const ComPtr& a, std::nullptr_t) noexcept { return a.ptr_ != nullptr; }
    friend bool operator==(std::nullptr_t, const ComPtr& a) noexcept { return nullptr == a.ptr_; }
    friend bool operator!=(std::nullptr_t, const ComPtr& a) noexcept { return nullptr != a.ptr_; }
};

} // namespace WRL
} // namespace Microsoft
