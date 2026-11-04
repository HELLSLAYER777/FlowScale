// FlowScale entry point.
#include "app/App.h"
#include "util/Common.h"
#include "util/Log.h"
#include <shellapi.h>
#include <shlobj.h>
#include <winrt/base.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

namespace {
// Fills fs::AppDataDir()/ShaderDir()/ModelsDir() results once.
struct Paths {
    std::wstring appData, shaderDir, modelsDir;
    Paths() {
        wchar_t base[MAX_PATH]{};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, base))) {
            appData = std::wstring(base) + L"\\FlowScale";
            CreateDirectoryW(appData.c_str(), nullptr);
        }
        wchar_t exe[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe, MAX_PATH);
        std::wstring dir = exe;
        const size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos) dir.resize(slash);
        shaderDir = dir + L"\\shaders";
        modelsDir = dir + L"\\models";
        CreateDirectoryW(modelsDir.c_str(), nullptr);
    }
};
} // namespace

namespace fs {
static Paths g_paths;
std::wstring AppDataDir() { return g_paths.appData; }
std::wstring ShaderDir()  { return g_paths.shaderDir; }
std::wstring ModelsDir()  { return g_paths.modelsDir; }
std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    std::wstring out(size_t(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), out.data(), n);
    return out;
}
std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(size_t(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), out.data(), n, nullptr, nullptr);
    return out;
}
} // namespace fs

int APIENTRY wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    UNREFERENCED_PARAMETER(show);

    // Single instance.
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"FlowScale.SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"FlowScale is already running.", L"FlowScale", MB_ICONINFORMATION);
        return 0;
    }

    // Per-monitor DPI awareness (topmost presenter geometry depends on it).
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // COM/WinRT apartment: MTA. Windows.Graphics.Capture factory activation
    // (RoGetActivationFactory / CreateFreeThreaded) needs an initialized
    // apartment, and MTA keeps the WGC threadpool handler free-threaded.
    try { winrt::init_apartment(winrt::apartment_type::multi_threaded); }
    catch (...) { /* keep going: DDA and the flow pipeline work without WinRT */ }

    fs::Log::OpenFile(fs::AppDataDir() + L"\\flowscale.log");
    LOGI("FlowScale %s starting", FS_VERSION);

    fs::App app;
    const int rc = app.Run(inst);

    if (mutex) { ReleaseMutex(mutex); CloseHandle(mutex); }
    return rc;
}
