#include "runtime.h"
#include "win32.h"

begin_c

#ifdef WINDOWS

#ifndef OutputDebugString // beats #include <Windows.h>
__declspec(dllimport) void __stdcall OutputDebugStringW(const uint16_t* s);
#pragma comment(lib, "Kernel32")
#endif

static const char* debug_abbreviate(const char* file) {
    const char* fn = strrchr(file, '\\');
    if (fn == null) { fn = strrchr(file, '/'); }
    return fn != null ? fn + 1 : file;
}

static void debug_vprintf(const char* file, int line, const char* func,
        const char* format, va_list vl) {
    char prefix[2 * 1024];
    // full path is useful in MSVC debugger output pane (clickable)
    // for all other scenarious short filename without path is preferable:
    const char* name = IsDebuggerPresent() ? file : debug_abbreviate(file);
    // snprintf() does not guarantee zero termination on truncation
    snprintf(prefix, countof(prefix) - 1, "%s(%d): %s", name, line, func);
    prefix[countof(prefix) - 1] = 0; // zero terminated
    char text[2 * 1024];
    if (format != null && !strequ(format, "")) {
        vsnprintf(text, countof(text) - 1, format, vl);
        text[countof(text) - 1] = 0;
    } else {
        text[0] = 0;
    }
    char output[4 * 1024];
    snprintf(output, countof(output) - 1, "%s %s", prefix, text);
    output[countof(output) - 2] = 0;
    // strip trailing \n which can be remnant of fprintf("...\n")
    int32_t n = (int32_t)strlen(output);
    while (n > 0 && (output[n - 1] == '\n' || output[n - 1] == '\r')) {
        output[n - 1] = 0;
        n--;
    }
    // For link.exe /Subsystem:Windows code stdout/stderr are often closed
    if (stderr != null && fileno(stderr) >= 0) {
        fprintf(stderr, "%s\n", output);
    }
    assert(n + 1 < countof(output));
    // OutputDebugString() needs \n
    output[n + 0] = '\n';
    output[n + 1] = 0;
    // SetConsoleCP(CP_UTF8) is not guaranteed to be called
    OutputDebugStringW(utf8to16(output));
}

#else // posix version:

static void debug_vprintf(const char* file, int line, const char* func,
        const char* format, va_list vl) {
    fprintf(stderr, "%s(%d): %s ", file, line, func);
    vfprintf(stderr, format, vl);
    fprintf(stderr, "\n");
}

#endif

static void debug_perrno(const char* file, int32_t line,
    const char* func, int32_t err_no, const char* format, ...) {
    if (err_no != 0) {
        if (format != null && !strequ(format, "")) {
            va_list vl;
            va_start(vl, format);
            debug.vprintf(file, line, func, format, vl);
            va_end(vl);
        }
        debug.printf(file, line, func, "errno: %d %s", err_no, strerror(err_no));
    }
}

static void debug_perror(const char* file, int32_t line,
    const char* func, int32_t error, const char* format, ...) {
    if (error != 0) {
        if (format != null && !strequ(format, "")) {
            va_list vl;
            va_start(vl, format);
            debug.vprintf(file, line, func, format, vl);
            va_end(vl);
        }
        debug.printf(file, line, func, "error: %s", str.error(error));
    }
}

static void debug_printf(const char* file, int line, const char* func,
        const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    debug.vprintf(file, line, func, format, vl);
    va_end(vl);
}

static void debug_breakpoint(void) { if (IsDebuggerPresent()) { DebugBreak(); } }

debug_if debug = {
    .verbosity = 0,
    .info  =  1,
    .warn  =  2,
    .error =  3,
    .fatal =  4,
    .debug =  5,
    .trace =  6,
    .printf  = debug_printf,
    .vprintf = debug_vprintf,
    .perrno  = debug_perrno,
    .perror  = debug_perror,
    .breakpoint = debug_breakpoint
};

end_c
