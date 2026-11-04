// FlowScale CI syntax-check stub of the Windows SDK (compile-time surface only).
// Mirrors the REAL signatures of every API the FlowScale sources use so that
// `g++ -fsyntax-only` catches the same errors MSVC would (arity, types, names).
// NOT for linking or execution. Values of handles/constants are irrelevant.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE   // _wfopen and friends on glibc
#endif
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <cstdarg>
#include <string>
#include <algorithm>

// ------------------------------------------------------------------ basics --
#define WINAPI
#define CALLBACK
#define APIENTRY
#ifndef __cdecl
#define __cdecl   // no-op on x64 (matches MSVC)
#endif
#ifndef __stdcall
#define __stdcall
#endif
#define TRUE  1
#define FALSE 0
typedef int             BOOL;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int    UINT;
typedef int             INT;
typedef long            LONG;
typedef unsigned long   ULONG;
typedef unsigned long   DWORD;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned long long UINT64;
typedef size_t          SIZE_T;
typedef void*           LPVOID;
typedef const void*     LPCVOID;
typedef void*           HANDLE;
typedef HANDLE          HMODULE;
typedef HANDLE          HINSTANCE;
typedef HANDLE          HWND;
typedef HANDLE          HMONITOR;
typedef HANDLE          HMENU;
typedef HANDLE          HDC;
typedef HANDLE          HBRUSH;
typedef HANDLE          HCURSOR;
typedef HANDLE          HICON;
typedef HANDLE          HFONT;
typedef HANDLE          HGLOBAL;
typedef HANDLE          DPI_AWARENESS_CONTEXT;
struct HSTRING__;
typedef HSTRING__*      HSTRING;
typedef DWORD*          LPDWORD;
typedef wchar_t*        LPWSTR;
typedef const wchar_t*  LPCWSTR;
typedef char*           LPSTR;
typedef const char*     LPCSTR;
typedef uintptr_t       UINT_PTR;
typedef intptr_t        LONG_PTR;
typedef uintptr_t       ULONG_PTR;
typedef ULONG_PTR       DWORD_PTR;
typedef UINT_PTR        WPARAM;   // real winuser.h: UINT_PTR (64-bit on x64)
typedef LONG_PTR        LPARAM;
typedef LONG_PTR        LRESULT;
typedef unsigned long   HRESULT;
typedef float           FLOAT;
typedef double          DOUBLE;
typedef wchar_t         WCHAR;
typedef wchar_t*        PWSTR;
typedef const wchar_t*  PCWSTR;
typedef DWORD           COLORREF;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

struct GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
};
typedef GUID IID;
typedef const IID& REFIID;

struct LUID { unsigned long LowPart; long HighPart; };

union LARGE_INTEGER {
    struct { unsigned long LowPart; long HighPart; } u;
    long long QuadPart;
};
union ULARGE_INTEGER {
    struct { unsigned long LowPart; long HighPart; } u;
    unsigned long long QuadPart;
};

struct RECT { long left, top, right, bottom; };
struct POINT { long x, y; };
struct SIZE { long cx, cy; };

#define FAILED(hr)       (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr)    (((HRESULT)(hr)) >= 0)
const HRESULT S_OK = 0x00000000u;
const HRESULT S_FALSE = 0x00000001u;
const HRESULT E_FAIL = 0x80004005u;
const HRESULT E_NOINTERFACE = 0x80004002u;
const HRESULT E_INVALIDARG = 0x80070057u;
const HRESULT E_POINTER = 0x80004003u;
const HRESULT E_NOTIMPL = 0x80004001u;

#define UNREFERENCED_PARAMETER(x) ((void)(x))
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

// ------------------------------------------------------- COM core (unknwn) --
struct IUnknown {
    virtual HRESULT QueryInterface(REFIID riid, void** ppvObject) { (void)riid; (void)ppvObject; return E_NOINTERFACE; }
    virtual unsigned long AddRef() { return 1; }
    virtual unsigned long Release() { return 0; }
    virtual ~IUnknown() = default;
};
struct IClassFactory : IUnknown {};
struct IInspectable : IUnknown {};

// __uuidof emulation for g++: works on type-ids and expressions.
template <typename T> struct FsUuid;   // specialized per interface below (FS_UUID)
template <typename T> struct FsUuidOfT {
    static constexpr GUID value = FsUuid<T>::value;
};
#define __uuidof(x) (::FsUuidOfT<x>::value)

#define FS_UUID(IFACE, a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)                 \
    template <> struct FsUuid<IFACE> {                                          \
        static constexpr GUID value = {a, b, c, {d0, d1, d2, d3, d4, d5, d6, d7}}; \
    };

template <typename T> inline void** FsIidPpvArgsHelper(T** pp) noexcept {
    return reinterpret_cast<void**>(pp);
}
template <typename T> constexpr GUID FsGuidOfPtr(T**) noexcept {
    return FsUuid<std::remove_cv_t<T>>::value;
}
#define IID_PPV_ARGS(pp) ::FsGuidOfPtr(pp), ::FsIidPpvArgsHelper(pp)

// ------------------------------------------------------------- MSVC CRT shim --
#define _TRUNCATE ((size_t)-1)
typedef int errno_t;
inline int vsnprintf_s(char* dst, size_t n, size_t /*truncate*/, const char* fmt, va_list ap) {
    return vsnprintf(dst, n, fmt, ap);
}
template <size_t N, typename... A>
inline int swprintf_s(wchar_t (&dst)[N], const wchar_t* fmt, A... args) {
    return swprintf(dst, N, fmt, args...);
}
inline errno_t _wfopen_s(FILE** f, const wchar_t* name, const wchar_t* mode) {
    (void)name; (void)mode;
    *f = fopen("flowscale_stub", "w");   // value irrelevant for syntax check
    return *f ? 0 : 1;
}
inline int MultiByteToWideChar(UINT cp, DWORD flags, const char* src, int slen, wchar_t* dst, int dlen) {
    (void)cp; (void)flags; (void)src; (void)slen; (void)dst; (void)dlen; return 0;
}
inline int WideCharToMultiByte(UINT cp, DWORD flags, const wchar_t* src, int slen, char* dst, int dlen,
                               const char* defchar, int* used) {
    (void)cp; (void)flags; (void)src; (void)slen; (void)dst; (void)dlen; (void)defchar; (void)used; return 0;
}
inline int _wcsicmp(const wchar_t* a, const wchar_t* b) { return wcscasecmp(a, b); }
inline double _wtof(const wchar_t* s) { return wcstod(s, nullptr); }

// --------------------------------------------------------------- Win32 API --
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL    (*WNDENUMPROC)(HWND, LPARAM);
typedef void    (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);

struct WNDCLASSEXW {
    UINT cbSize; UINT style; WNDPROC lpfnWndProc; int cbClsExtra; int cbWndExtra;
    HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground;
    LPCWSTR lpszMenuName; LPCWSTR lpszClassName; HICON hIconSm;
};
struct CREATESTRUCTW {
    LPVOID lpCreateParams; HINSTANCE hInstance; HMENU hMenu; HWND hwndParent;
    int cy, cx, y, x; LONG style; LPCWSTR lpszName; LPCWSTR lpszClass; DWORD dwExStyle;
};
typedef CREATESTRUCTW* LPCREATESTRUCTW;
struct MSG {
    HWND hwnd; UINT message; UINT_PTR wParam; LPARAM lParam; DWORD time; POINT pt;
};
struct MONITORINFO {
    DWORD cbSize; RECT rcMonitor; RECT rcWork; DWORD dwFlags;
};
struct INITCOMMONCONTROLSEX { DWORD dwSize; DWORD dwICC; };

// window messages / styles / constants (values irrelevant)
enum : UINT {
    WM_NULL = 0, WM_CREATE = 1, WM_DESTROY = 2, WM_MOVE = 3, WM_SIZE = 5, WM_ACTIVATE = 6,
    WM_PAINT = 0x000F, WM_CLOSE = 0x0010, WM_QUIT = 0x0012, WM_ERASEBKGND = 0x0014,
    WM_SHOWWINDOW = 0x0018, WM_NCCREATE = 0x0081, WM_NCDESTROY = 0x0082, WM_NCCALCSIZE = 0x0083,
    WM_NCHITTEST = 0x0084, WM_NCPAINT = 0x0085, WM_NCACTIVATE = 0x0086,
    WM_CTLCOLORSTATIC = 0x0138, WM_CTLCOLORBTN = 0x0135, WM_HSCROLL = 0x0114, WM_VSCROLL = 0x0115,
    WM_KEYDOWN = 0x0100, WM_KEYUP = 0x0101, WM_CHAR = 0x0102, WM_TIMER = 0x0113,
    WM_COMMAND = 0x0111, WM_HOTKEY = 0x0312,
    WM_SETFONT = 0x0030, WM_GETFONT = 0x0031, WM_SETICON = 0x0080,
};
enum : long {
    WS_OVERLAPPED = 0x00000000, WS_POPUP = (long)0x80000000L, WS_CHILD = (long)0x40000000L,
    WS_VISIBLE = (long)0x10000000L, WS_CAPTION = 0x00C00000L, WS_SYSMENU = 0x00080000L,
    WS_THICKFRAME = 0x00040000L, WS_MINIMIZEBOX = 0x00020000L, WS_MAXIMIZEBOX = 0x00010000L,
    WS_OVERLAPPEDWINDOW = 0x00CF0000L,
    WS_EX_TOPMOST = 0x00000008L, WS_EX_TOOLWINDOW = 0x00000080L, WS_EX_NOACTIVATE = (long)0x08000000L,
    WS_EX_APPWINDOW = 0x00040000L,
};
enum : int { SW_HIDE = 0, SW_SHOWNORMAL = 1, SW_SHOW = 5, SW_MINIMIZE = 6, SW_SHOWNOACTIVATE = 4 };
enum : long { WS_VSCROLL = 0x00200000L, WS_BORDER = 0x00800000L, WS_TABSTOP = 0x00010000L };
enum : UINT { CBS_SIMPLE = 0x1, CBS_DROPDOWN = 0x2, CBS_DROPDOWNLIST = 0x3 };
enum : UINT { BS_PUSHBUTTON = 0x0, BS_DEFPUSHBUTTON = 0x1, BS_AUTOCHECKBOX = 0x3 };
enum : UINT { ES_LEFT = 0x0, ES_CENTER = 0x1, ES_AUTOHSCROLL = 0x80, ES_NUMBER = 0x2000 };
enum : UINT { SWP_NOSIZE = 0x0001, SWP_NOMOVE = 0x0002, SWP_NOACTIVATE = 0x0010 };
enum : int { HTCLIENT = 1, HTCAPTION = 2 };
enum : int { GWL_STYLE = -16, GWL_EXSTYLE = -20, GWLP_USERDATA = -21, GWLP_WNDPROC = -4 };
enum : UINT { VK_ESCAPE = 0x1B, VK_F5 = 0x74 };
enum : int { MONITOR_DEFAULTTONEAREST = 2, MONITOR_DEFAULTTONULL = 0 };
enum : UINT { MOD_ALT = 0x1, MOD_CONTROL = 0x2, MOD_SHIFT = 0x4, MOD_NOREPEAT = 0x4000 };
enum : UINT { BST_UNCHECKED = 0, BST_CHECKED = 1 };
enum : UINT { MB_ICONINFORMATION = 0x40, MB_ICONWARNING = 0x30, MB_OK = 0x0 };
enum : UINT { BN_CLICKED = 0, EN_CHANGE = 0x300, CBN_SELCHANGE = 1 };
#define CW_USEDEFAULT ((int)0x80000000)
#define IDC_ARROW   ((LPCWSTR)(ULONG_PTR)32512)
#define IDI_APPLICATION ((LPCWSTR)(ULONG_PTR)32512)
#define HWND_TOPMOST  ((HWND)(ULONG_PTR)-1)
#define HWND_NOTOPMOST ((HWND)(ULONG_PTR)-2)
#define WAIT_OBJECT_0 0u
#define INFINITE      0xFFFFFFFFu
#define INVALID_HANDLE_VALUE ((HANDLE)(ULONG_PTR)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)0xFFFFFFFFu)
#define ERROR_ALREADY_EXISTS 183u
#define ERROR_SUCCESS 0u
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#define CP_UTF8 65001u
#define CSIDL_LOCAL_APPDATA 0x001c
#define ICC_STANDARD_CLASSES 0x4000
#define ICC_BAR_CLASSES 0x0004
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)(ULONG_PTR)-4)
#define MAKELPARAM(l, h) ((LPARAM)(((WORD)(((ULONG_PTR)(l)) & 0xffff)) | \
                          ((unsigned long)((WORD)(((ULONG_PTR)(h)) & 0xffff))) << 16))
#define MAKEINTRESOURCEW(i) ((LPCWSTR)((ULONG_PTR)(i)))
#define LOWORD(l) ((WORD)(((ULONG_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((ULONG_PTR)(l)) >> 16) & 0xffff))
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g))) << 8 | ((DWORD)((BYTE)(b))) << 16))

// --- kernel32 ---
inline HMODULE GetModuleHandleW(LPCWSTR) { return nullptr; }
inline DWORD GetModuleFileNameW(HMODULE, LPWSTR out, DWORD n) { if (n) out[0] = 0; return 0; }
inline HMODULE LoadLibraryW(LPCWSTR) { return nullptr; }
inline void* GetProcAddress(HMODULE, const char*) { return nullptr; }
inline BOOL FreeLibrary(HMODULE) { return TRUE; }
inline HANDLE CreateMutexW(void*, BOOL, LPCWSTR) { return nullptr; }
inline BOOL ReleaseMutex(HANDLE) { return TRUE; }
inline DWORD GetLastError() { return 0; }
inline HANDLE OpenProcess(DWORD, BOOL, DWORD) { return nullptr; }
inline BOOL CloseHandle(HANDLE) { return TRUE; }
inline BOOL QueryFullProcessImageNameW(HANDLE, DWORD, LPWSTR, DWORD* len) { if (len) *len = 0; return FALSE; }
inline void Sleep(DWORD) {}
inline DWORD WaitForSingleObject(HANDLE, DWORD) { return 0; }
inline int QueryPerformanceCounter(LARGE_INTEGER* c) { if (c) c->QuadPart = 0; return 1; }
inline int QueryPerformanceFrequency(LARGE_INTEGER* f) { if (f) f->QuadPart = 10000000; return 1; }
inline void OutputDebugStringA(LPCSTR) {}
inline DWORD GetFileAttributesW(LPCWSTR) { return INVALID_FILE_ATTRIBUTES; }
inline BOOL CreateDirectoryW(LPCWSTR, void*) { return TRUE; }
inline HANDLE GetCurrentProcess() { return nullptr; }
inline DWORD GetCurrentProcessId() { return 0; }
inline BOOL IsDebuggerPresent() { return FALSE; }
inline void OutputDebugStringW(LPCWSTR) {}

// --- user32 / gdi32 ---
typedef unsigned short ATOM;
inline ATOM RegisterClassExW(const WNDCLASSEXW*) { return 0; }
inline BOOL UnregisterClassW(LPCWSTR, HINSTANCE) { return TRUE; }
inline HWND CreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int,
                            HWND, HMENU, HINSTANCE, LPVOID) { return nullptr; }
inline BOOL DestroyWindow(HWND) { return TRUE; }
inline BOOL ShowWindow(HWND, int) { return TRUE; }
inline BOOL UpdateWindow(HWND) { return TRUE; }
inline BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return TRUE; }
inline LRESULT DefWindowProcW(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline LONG_PTR GetWindowLongPtrW(HWND, int) { return 0; }
inline LONG_PTR SetWindowLongPtrW(HWND, int, LONG_PTR) { return 0; }
inline LONG GetWindowLongW(HWND, int) { return 0; }
inline BOOL EnumWindows(WNDENUMPROC, LPARAM) { return TRUE; }
inline BOOL IsWindowVisible(HWND) { return TRUE; }
inline int GetWindowTextW(HWND, LPWSTR, int) { return 0; }
inline int GetClassNameW(HWND, LPWSTR, int) { return 0; }
inline DWORD GetWindowThreadProcessId(HWND, DWORD*) { return 0; }
inline HWND GetDlgItem(HWND, int) { return nullptr; }
inline LRESULT SendDlgItemMessageW(HWND, int, UINT, WPARAM, LPARAM) { return 0; }
inline BOOL SetDlgItemTextW(HWND, int, LPCWSTR) { return TRUE; }
inline UINT GetDlgItemTextW(HWND, int, LPWSTR, int) { return 0; }
inline BOOL SetDlgItemInt(HWND, int, UINT, BOOL) { return TRUE; }
inline UINT GetDlgItemInt(HWND, int, BOOL*, BOOL) { return 0; }
inline BOOL CheckDlgButton(HWND, int, UINT) { return TRUE; }
inline UINT IsDlgButtonChecked(HWND, int) { return 0; }
inline BOOL SetWindowTextW(HWND, LPCWSTR) { return TRUE; }
inline int GetWindowTextLengthW(HWND) { return 0; }
inline BOOL RegisterHotKey(HWND, int, UINT, UINT) { return TRUE; }
inline BOOL UnregisterHotKey(HWND, int) { return TRUE; }
inline UINT_PTR SetTimer(HWND, UINT_PTR, UINT, TIMERPROC) { return 1; }
inline BOOL KillTimer(HWND, UINT_PTR) { return TRUE; }
inline LRESULT SendMessageW(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline LRESULT PostMessageW(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline int GetMessageW(MSG*, HWND, UINT, UINT) { return 0; }
inline BOOL TranslateMessage(const MSG*) { return TRUE; }
inline LRESULT DispatchMessageW(const MSG*) { return 0; }
inline void PostQuitMessage(int) {}
inline int MessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT) { return 0; }
inline HWND MonitorFromWindow(HWND, DWORD) { return nullptr; }
inline BOOL GetMonitorInfoW(HMONITOR, MONITORINFO*) { return FALSE; }
inline HCURSOR LoadCursorW(HINSTANCE, LPCWSTR) { return nullptr; }
inline HICON LoadIconW(HINSTANCE, LPCWSTR) { return nullptr; }
inline HBRUSH CreateSolidBrush(COLORREF) { return nullptr; }
inline BOOL DeleteObject(HANDLE) { return TRUE; }
// --- gdi32 font surface (wingdi.h in the real SDK) ---
enum : UINT { FW_NORMAL = 400, FW_SEMI_BOLD = 600, FW_BOLD = 700 };
enum : UINT { DEFAULT_CHARSET = 1, OUT_DEFAULT_PRECIS = 0, CLIP_DEFAULT_PRECIS = 0,
              CLEARTYPE_QUALITY = 5, DEFAULT_PITCH = 0, FF_DONTCARE = 0 };
inline HFONT CreateFontW(int cHeight, int cWidth, int cEscapement, int cOrientation, UINT cWeight,
                         DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, UINT iCharSet,
                         UINT iOutPrecision, UINT iClipPrecision, UINT iQuality, UINT iPitchAndFamily,
                         LPCWSTR pszFaceName) {
    (void)cHeight; (void)cWidth; (void)cEscapement; (void)cOrientation; (void)cWeight;
    (void)bItalic; (void)bUnderline; (void)bStrikeOut; (void)iCharSet; (void)iOutPrecision;
    (void)iClipPrecision; (void)iQuality; (void)iPitchAndFamily; (void)pszFaceName;
    return nullptr;
}
inline COLORREF SetTextColor(HDC, COLORREF) { return 0; }
inline COLORREF SetBkColor(HDC, COLORREF) { return 0; }
inline BOOL SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT) { return TRUE; }
inline HWND SetFocus(HWND) { return nullptr; }
inline BOOL EnableWindow(HWND, BOOL) { return TRUE; }
