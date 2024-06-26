#include "ut/ut.h"
#include "ut/ut_win32.h"

static char* ut_str_drop_const(const char* s) {
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-qual"
    #endif
    return (char*)s;
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
}

static int32_t ut_str_len(const char* s) { return (int32_t)strlen(s); }

static int32_t ut_str_utf16len(const uint16_t* utf16) {
    return (int32_t)wcslen(utf16);
}

static void ut_str_lower(char* d, int32_t capacity, const char* s) {
    int32_t n = ut_str.len(s);
    swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)tolower(s[i]); }
    d[n] = 0;
}

static void ut_str_upper(char* d, int32_t capacity, const char* s) {
    int32_t n = ut_str.len(s);
    swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)toupper(s[i]); }
    d[n] = 0;
}

static bool ut_str_starts(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1, s2, n2) == 0;
}

static bool ut_str_ends(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1 + n1 - n2, s2, n2) == 0;
}

static bool ut_str_i_starts(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strnicmp(s1, s2, n2) == 0;

}

static bool ut_str_i_ends(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strnicmp(s1 + n1 - n2, s2, n2) == 0;
}

static int32_t ut_str_utf8_bytes(const uint16_t* utf16) {
    // If cchWideChar argument is -1, the function WideCharToMultiByte
    // includes the zero-terminating character in the conversion and
    // the returned byte count.
    int32_t required_bytes_count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    swear(required_bytes_count < 0 || required_bytes_count > 0);
    return required_bytes_count;
}

static int32_t ut_str_utf16_chars(const char* utf8) {
    // If cbMultiByte argument is -1, the function MultiByteToWideChar
    // includes the zero-terminating character in the conversion and
    // the returned byte count.
    int32_t required_wide_chars_count =
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    swear(required_wide_chars_count < 0 || required_wide_chars_count > 0);
    return required_wide_chars_count;
}

static void ut_str_utf16to8(char* d, int32_t capacity, const uint16_t* utf16) {
    const int32_t required = ut_str.utf8_bytes(utf16);
    swear(required > 0 && capacity >= required);
    int32_t bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                        utf16, -1, d, capacity, null, null);
    swear(required == bytes);
}

static void ut_str_utf8to16(uint16_t* d, int32_t capacity, const char* utf8) {
    const int32_t required = ut_str.utf16_chars(utf8);
    swear(required > 0 && capacity >= required);
    int32_t count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, d, capacity);
    swear(required == count);
}

static void ut_str_format_va(char* utf8, int32_t count, const char* format,
        va_list va) {
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #endif
    vsnprintf(utf8, (size_t)count, format, va);
    utf8[count - 1] = 0;
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
}

static void ut_str_format(char* utf8, int32_t count, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ut_str.format_va(utf8, count, format, va);
    va_end(va);
}

static str1024_t ut_str_error_for_language(int32_t error, LANGID language) {
    DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    HMODULE module = null;
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32((uint32_t)error) : (HRESULT)error;
    if ((error & 0xC0000000U) == 0xC0000000U) {
        // https://stackoverflow.com/questions/25566234/how-to-convert-specific-ntstatus-value-to-the-hresult
        static HMODULE ntdll; // RtlNtStatusToDosError implies linking to ntdll
        if (ntdll == null) { ntdll = GetModuleHandleA("ntdll.dll"); }
        if (ntdll == null) { ntdll = LoadLibraryA("ntdll.dll"); }
        module = ntdll;
        hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError((NTSTATUS)error));
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    str1024_t text;
    uint16_t utf16[countof(text.s)];
    DWORD count = FormatMessageW(flags, module, hr, language,
            utf16, countof(utf16) - 1, (va_list*)null);
    utf16[countof(utf16) - 1] = 0; // always
    // If FormatMessageW() succeeds, the return value is the number of utf16
    // characters stored in the output buffer, excluding the terminating zero.
    if (count > 0) {
        swear(count < countof(utf16));
        utf16[count] = 0;
        // remove trailing '\r\n'
        int32_t k = count;
        if (k > 0 && utf16[k - 1] == '\n') { utf16[k - 1] = 0; }
        k = (int32_t)ut_str.len16(utf16);
        if (k > 0 && utf16[k - 1] == '\r') { utf16[k - 1] = 0; }
        char message[countof(text.s)];
        const int32_t bytes = ut_str.utf8_bytes(utf16);
        if (bytes >= countof(message)) {
            strprintf(message, "error message is too long: %d bytes", bytes);
        } else {
            ut_str.utf16to8(message, countof(message), utf16);
        }
        // truncating printf to string:
        ut_str_printf(text.s, "0x%08X(%d) \"%s\"", error, error, message);
    } else {
        ut_str_printf(text.s, "0x%08X(%d)", error, error);
    }
    return text;
}

static str1024_t ut_str_error(int32_t error) {
    const LANGID language = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return ut_str_error_for_language(error, language);
}

static str1024_t ut_str_error_nls(int32_t error) {
    const LANGID language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return ut_str_error_for_language(error, language);
}

static const char* ut_str_grouping_separator(void) {
    #ifdef WINDOWS
        // en-US Windows 10/11:
        // grouping_separator == ","
        // decimal_separator  == "."
        static char grouping_separator[8];
        if (grouping_separator[0] == 0x00) {
            errno_t r = ut_b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
                grouping_separator, sizeof(grouping_separator)));
            swear(r == 0 && grouping_separator[0] != 0);
        }
        return grouping_separator;
    #else
        // en-US Windows 10/11:
        // grouping_separator == ""
        // decimal_separator  == "."
        struct lconv *locale_info = localeconv();
        const char* grouping_separator = null;
        if (grouping_separator == null) {
            grouping_separator = locale_info->thousands_sep;
            swear(grouping_separator != null);
        }
        return grouping_separator;
    #endif
}

// Posix and Win32 C runtime:
//   #include <locale.h>
//   struct lconv *locale_info = localeconv();
//   const char* grouping_separator = locale_info->thousands_sep;
//   const char* decimal_separator = locale_info->decimal_point;
// en-US Windows 1x:
// grouping_separator == ""
// decimal_separator  == "."
//
// Win32 API:
//   ut_b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
//       grouping_separator, sizeof(grouping_separator)));
//   ut_b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
//       decimal_separator, sizeof(decimal_separator)));
// en-US Windows 1x:
// grouping_separator == ","
// decimal_separator  == "."

static str64_t ut_str_int64_dg(int64_t v, // digit_grouped
        bool uint, const char* gs) { // grouping separator: gs
    // sprintf format %`lld may not be implemented or
    // does not respect locale or UI separators...
    // Do it hard way:
    const int32_t m = (int32_t)strlen(gs);
    swear(m < 5); // utf-8 4 bytes max
    // 64 calls per thread 32 or less bytes each because:
    // "18446744073709551615" 21 characters + 6x4 groups:
    // "18'446'744'073'709'551'615" 27 characters
    str64_t text;
    enum { max_text_bytes = countof(text.s) };
    int64_t abs64 = v < 0 ? -v : v; // incorrect for INT64_MIN
    uint64_t n = uint ? (uint64_t)v :
        (v != INT64_MIN ? (uint64_t)abs64 : (uint64_t)INT64_MIN);
    int32_t i = 0;
    int32_t groups[8]; // 2^63 - 1 ~= 9 x 10^19 upto 7 groups of 3 digits
    do {
        groups[i] = n % 1000;
        n = n / 1000;
        i++;
    } while (n > 0);
    const int32_t gc = i - 1; // group count
    char* s = text.s;
    if (v < 0 && !uint) { *s++ = '-'; } // sign
    int32_t r = max_text_bytes - 1;
    while (i > 0) {
        i--;
        assert(r > 3 + m);
        if (i == gc) {
            ut_str.format(s, r, "%d%s", groups[i], gc > 0 ? gs : "");
        } else {
            ut_str.format(s, r, "%03d%s", groups[i], i > 0 ? gs : "");
        }
        int32_t k = (int32_t)strlen(s);
        r -= k;
        s += k;
    }
    *s = 0;
    return text;
}

static str64_t ut_str_int64(int64_t v) {
    return ut_str_int64_dg(v, false, ut_glyph_hair_space);
}

static str64_t ut_str_uint64(uint64_t v) {
    return ut_str_int64_dg(v, true, ut_glyph_hair_space);
}

static str64_t ut_str_int64_lc(int64_t v) {
    return ut_str_int64_dg(v, false, ut_str_grouping_separator());
}

static str64_t ut_str_uint64_lc(uint64_t v) {
    return ut_str_int64_dg(v, true, ut_str_grouping_separator());
}

static str128_t ut_str_fp(const char* format, fp64_t v) {
    static char decimal_separator[8];
    if (decimal_separator[0] == 0) {
        errno_t r = ut_b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
            decimal_separator, sizeof(decimal_separator)));
        swear(r == 0 && decimal_separator[0] != 0);
    }
    swear(strlen(decimal_separator) <= 4);
    str128_t f; // formatted float point
    // snprintf format does not handle thousands separators on all know runtimes
    // and respects setlocale() on Un*x systems but in MS runtime only when
    // _snprintf_l() is used.
    f.s[0] = 0x00;
    ut_str.format(f.s, countof(f.s), format, v);
    f.s[countof(f.s) - 1] = 0x00;
    str128_t text;
    char* s = f.s;
    char* d = text.s;
    while (*s != 0x00) {
        if (*s == '.') {
            const char* sep = decimal_separator;
            while (*sep != 0x00) { *d++ = *sep++; }
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = 0x00;
    // TODO: It's possible to handle mantissa grouping but...
    // Not clear if human expects it in 5 digits or 3 digits chunks
    // and unfortunately locale does not specify how
    return text;
}

#ifdef UT_TESTS

static void ut_str_test(void) {
    swear(ut_str.len("hello") == 5);
    swear(ut_str.starts("hello world", "hello"));
    swear(ut_str.ends("hello world", "world"));
    swear(ut_str.istarts("hello world", "HeLlO"));
    swear(ut_str.iends("hello world", "WoRlD"));
    char ls[20] = {0};
    ut_str.lower(ls, countof(ls), "HeLlO WoRlD");
    swear(strcmp(ls, "hello world") == 0);
    char upper[11] = {0};
    ut_str.upper(upper, countof(upper), "hello12345");
    swear(strcmp(upper,  "HELLO12345") == 0);
    #pragma push_macro("glyph_chinese_one")
    #pragma push_macro("glyph_chinese_two")
    #pragma push_macro("glyph_teddy_bear")
    #pragma push_macro("glyph_ice_cube")
    #define glyph_chinese_one "\xE5\xA3\xB9"
    #define glyph_chinese_two "\xE8\xB4\xB0"
    #define glyph_teddy_bear  "\xF0\x9F\xA7\xB8"
    #define glyph_ice_cube    "\xF0\x9F\xA7\x8A"
    const char* utf8_str =
            glyph_teddy_bear
            "0"
            ut_glyph_chinese_jin4 ut_glyph_chinese_gong
            "3456789 "
            glyph_ice_cube;
    #pragma pop_macro("glyph_ice_cube")
    #pragma pop_macro("glyph_teddy_bear")
    #pragma pop_macro("glyph_chinese_two")
    #pragma pop_macro("glyph_chinese_one")
    uint16_t wide_str[100] = {0};
    ut_str.utf8to16(wide_str, countof(wide_str), utf8_str);
    char utf8[100] = {0};
    ut_str.utf16to8(utf8, countof(utf8), wide_str);
    uint16_t utf16[100];
    ut_str.utf8to16(utf16, countof(utf16), utf8);
    char narrow_str[100] = {0};
    ut_str.utf16to8(narrow_str, countof(narrow_str), utf16);
    swear(strcmp(narrow_str, utf8_str) == 0);
    char formatted[100];
    ut_str.format(formatted, countof(formatted), "n: %d, s: %s", 42, "test");
    swear(strcmp(formatted, "n: 42, s: test") == 0);
    // numeric values digit grouping format:
    swear(strcmp("0", ut_str.int64_dg(0, true, ",").s) == 0);
    swear(strcmp("-1", ut_str.int64_dg(-1, false, ",").s) == 0);
    swear(strcmp("999", ut_str.int64_dg(999, true, ",").s) == 0);
    swear(strcmp("-999", ut_str.int64_dg(-999, false, ",").s) == 0);
    swear(strcmp("1,001", ut_str.int64_dg(1001, true, ",").s) == 0);
    swear(strcmp("-1,001", ut_str.int64_dg(-1001, false, ",").s) == 0);
    swear(strcmp("18,446,744,073,709,551,615",
        ut_str.int64_dg(UINT64_MAX, true, ",").s) == 0
    );
    swear(strcmp("9,223,372,036,854,775,807",
        ut_str.int64_dg(INT64_MAX, false, ",").s) == 0
    );
    swear(strcmp("-9,223,372,036,854,775,808",
        ut_str.int64_dg(INT64_MIN, false, ",").s) == 0
    );
    //  see:
    // https://en.wikipedia.org/wiki/Single-precision_floating-point_format
    uint32_t pi_fp32 = 0x40490FDBULL; // 3.14159274101257324
    swear(strcmp("3.141592741",
                ut_str.fp("%.9f", *(fp32_t*)&pi_fp32).s) == 0,
          "%s", ut_str.fp("%.9f", *(fp32_t*)&pi_fp32).s
    );
    //  3.141592741
    //  ********^ (*** true digits ^ first rounded digit)
    //    123456 (%.6f)
    //
    //  https://en.wikipedia.org/wiki/Double-precision_floating-point_format
    uint64_t pi_fp64 = 0x400921FB54442D18ULL;
    swear(strcmp("3.141592653589793116",
                ut_str.fp("%.18f", *(fp64_t*)&pi_fp64).s) == 0,
          "%s", ut_str.fp("%.18f", *(fp64_t*)&pi_fp64).s
    );
    //  3.141592653589793116
    //  *****************^ (*** true digits ^ first rounded digit)
    //    123456789012345 (%.15f)
    //  https://en.wikipedia.org/wiki/Double-precision_floating-point_format
    //
    //  actual "pi" first 64 digits:
    //  3.1415926535897932384626433832795028841971693993751058209749445923
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_str_test(void) {}

#endif

ut_str_if ut_str = {
    .drop_const         = ut_str_drop_const,
    .len                = ut_str_len,
    .len16           = ut_str_utf16len,
    .lower              = ut_str_lower,
    .upper              = ut_str_upper,
    .starts             = ut_str_starts,
    .ends               = ut_str_ends,
    .istarts           = ut_str_i_starts,
    .iends             = ut_str_i_ends,
    .utf8_bytes         = ut_str_utf8_bytes,
    .utf16_chars        = ut_str_utf16_chars,
    .utf16to8           = ut_str_utf16to8,
    .utf8to16           = ut_str_utf8to16,
    .format             = ut_str_format,
    .format_va          = ut_str_format_va,
    .error              = ut_str_error,
    .error_nls          = ut_str_error_nls,
    .grouping_separator = ut_str_grouping_separator,
    .int64_dg           = ut_str_int64_dg,
    .int64              = ut_str_int64,
    .uint64             = ut_str_uint64,
    .int64_lc           = ut_str_int64,
    .uint64_lc          = ut_str_uint64,
    .fp                 = ut_str_fp,
    .test               = ut_str_test
};
