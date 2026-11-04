// FlowScale — minimal thread-safe logger (debugger output + optional file).
#pragma once
#include <windows.h>
#include <cstdio>
#include <mutex>
#include <string>

namespace fs {

enum class LogLevel { Debug, Info, Warn, Error };

class Log {
public:
    static void OpenFile(const std::wstring& path) {
        std::lock_guard<std::mutex> g(Mutex());
        if (s_file) { fclose(s_file); s_file = nullptr; }
        if (!path.empty()) _wfopen_s(&s_file, path.c_str(), L"w");
    }

    static void Write(LogLevel lvl, const char* fmt, ...) {
        char body[1024];
        va_list ap; va_start(ap, fmt);
        vsnprintf_s(body, sizeof(body), _TRUNCATE, fmt, ap);
        va_end(ap);

        static const char* tags[] = { "[debug]", "[info ]", "[warn ]", "[error]" };
        char line[1100];
        snprintf(line, sizeof(line), "%s %s\n", tags[static_cast<int>(lvl)], body);

        std::lock_guard<std::mutex> g(Mutex());
        OutputDebugStringA(line);
        if (s_file) { fputs(line, s_file); fflush(s_file); }
    }

private:
    static std::mutex& Mutex() { static std::mutex m; return m; }
    inline static FILE* s_file = nullptr;
};

#define LOGD(...) ::fs::Log::Write(::fs::LogLevel::Debug, __VA_ARGS__)
#define LOGI(...) ::fs::Log::Write(::fs::LogLevel::Info,  __VA_ARGS__)
#define LOGW(...) ::fs::Log::Write(::fs::LogLevel::Warn,  __VA_ARGS__)
#define LOGE(...) ::fs::Log::Write(::fs::LogLevel::Error, __VA_ARGS__)

} // namespace fs
