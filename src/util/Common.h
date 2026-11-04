// FlowScale — common utilities shared by every module.
#pragma once
#include <windows.h>
#include <dxgi1_4.h>   // base dxgi + 1_2 (Factory2/SwapChain1) + 1_3 (SwapChain2) + 1_4 (SwapChain3)
#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace fs {

template <typename T>
using CPtr = Microsoft::WRL::ComPtr<T>;

inline void ThrowIfFailed(HRESULT hr, const char* what) {
    if (FAILED(hr)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s failed: HRESULT 0x%08lX", what, static_cast<unsigned long>(hr));
        throw std::runtime_error(msg);
    }
}

template <typename T>
inline void SafeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

// Returns %APPDATA%\FlowScale, created on first call.
std::wstring AppDataDir();
// Returns <exe dir>\shaders (portable layout used by the shipped build).
std::wstring ShaderDir();
// Returns <exe dir>\models.
std::wstring ModelsDir();

std::wstring Utf8ToWide(const std::string& s);
std::string  WideToUtf8(const std::wstring& s);

} // namespace fs
