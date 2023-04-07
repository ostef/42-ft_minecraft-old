#include "Core.hpp"

#define CP_UTF8 65001

extern "C"
{
    void *GetStdHandle (u32 nStdHandle);
    int GetConsoleMode (void *hConsoleHandle, u32 *lpMode);
    int SetConsoleMode (void *hConsoleHandle, u32 dwMode);

    int QueryPerformanceFrequency (s64 *lpFrequency);
    int QueryPerformanceCounter (s64 *lpPerformanceCount);

    u32 GetModuleFileNameW (void *hModule, wchar_t *lpFilename, u32 nSize);

    int WideCharToMultiByte(
        u32      CodePage,
        u32      dwFlags,
        wchar_t *lpWideCharStr,
        int      cchWideChar,
        char    *lpMultiByteStr,
        int      cbMultiByte,
        char    *lpDefaultChar,
        int     *lpUsedDefaultChar
    );

    int MultiByteToWideChar(
        u32      CodePage,
        u32      dwFlags,
        char    *lpMultiByteStr,
        int      cbMultiByte,
        wchar_t *lpWideCharStr,
        int      cchWideChar
    );

    u32 GetFullPathNameW(
        wchar_t  *lpFileName,
        u32       nBufferLength,
        wchar_t  *lpBuffer,
        wchar_t **lpFilePart
    );

    u32 GetLastError ();
    u32 FormatMessageW (
        u32   dwFlags,
        const void *lpSource,
        u32   dwMessageId,
        u32  dwLanguageId,
        wchar_t *lpBuffer,
        u32 nSize,
        va_list *Arguments
    );

    void Sleep (
        u32 dwMilliseconds
    );
}

void platform_init ()
{
    // Enable virtual terminal sequences handling (otherwise we'll
    // get weird characters in cmd instead of nice colors)
    auto std_out = GetStdHandle (cast (u32) -11);
    auto std_err = GetStdHandle (cast (u32) -12);

    u32 mode;
    if (GetConsoleMode (std_out, &mode))
        SetConsoleMode (std_out, mode | 0x0004);

    if (GetConsoleMode (std_err, &mode))
        SetConsoleMode (std_err, mode | 0x0004);
}

s64 time_current_monotonic ()
{
    static s64 performance_frequency = 0;

    if (!performance_frequency)
        QueryPerformanceFrequency (&performance_frequency);

    s64 result;
    QueryPerformanceCounter (&result);
    result *= 1000000;	// Convert to micro seconds
    result /= performance_frequency;

    return result;
}

String get_executable_path ()
{
    static wchar_t wstr_path[1024];
    static String path;

    if (!path.data)
    {
        GetModuleFileNameW (null, wstr_path, array_size (wstr_path));
        path = wide_to_utf8 (wstr_path, heap_allocator ());
    }

    return path;
}

String wide_to_utf8 (wchar_t *data, Allocator allocator)
{
    int result_length = WideCharToMultiByte (CP_UTF8, 0, data, -1, null, 0, null, null);
    if (result_length <= 0)
        return string_make (0, null);

    char *utf8_data = mem_alloc_uninit (char, result_length, allocator);

    int written = WideCharToMultiByte (CP_UTF8, 0, data, -1, utf8_data, result_length, null, null);
    if (written > 0)
        return string_make (written - 1, utf8_data);

    return string_make (0, null);
}

wchar_t *utf8_to_wide (String str, s64 *out_length, Allocator allocator)
{
    if (str.count == 0)
    {
        wchar_t *wide_str = mem_alloc_uninit (wchar_t, 1, allocator);
        wide_str[0] = 0;
        if (out_length)
            *out_length = 0;

        return wide_str;
    }

    int result_length = MultiByteToWideChar (CP_UTF8, 0, str.data, cast (s32) str.count, null, 0);
    if (result_length <= 0)
    {
        if (out_length)
            *out_length = 0;

        return null;
    }

    wchar_t *wide_str = mem_alloc_uninit (wchar_t, result_length + 1, allocator);
    int written = MultiByteToWideChar (CP_UTF8, 0, str.data, cast (s32) str.count, wide_str, result_length);
    if (written > 0)
    {
        wide_str[written] = 0;
        if (out_length)
            *out_length = written;

        return wide_str;
    }

    if (out_length)
        *out_length = 0;

    return null;
}

String filename_get_full (String filename, Allocator allocator)
{
    wchar_t *wstr_filename = utf8_to_wide (filename, null, allocator);
    defer (mem_free (wstr_filename, allocator));

    u32 len = GetFullPathNameW (wstr_filename, 0, null, null);

    wchar_t *wstr_result = mem_alloc_uninit (wchar_t, len, allocator);
    defer (mem_free (wstr_result, allocator));

    len = GetFullPathNameW (wstr_filename, len, wstr_result, null);
    len += 1;

    String result = wide_to_utf8 (wstr_result, allocator);
    for_array (i, result)
    {
        if (result[i] == '\\')
            result[i] = '/';
    }

    return result;
}

String get_error_string (u32 error_code)
{
    static wchar_t error_wide_buffer[128];
    static char    error_utf8_buffer[512];

    u32 wide_count = FormatMessageW (
        0x1000 | 0x200, //FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        null,
        error_code,
        (1 << 10), //MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
        error_wide_buffer,
        sizeof (error_wide_buffer),
        null
    );

    int utf8_count = WideCharToMultiByte (
        CP_UTF8,
        0,
        error_wide_buffer,
        cast (int) wide_count,
        error_utf8_buffer,
        sizeof (error_utf8_buffer),
        null,
        null
    );

    return string_make (utf8_count, error_utf8_buffer);
}

String get_last_error_string ()
{
    return get_error_string (GetLastError ());
}

void sleep_milliseconds (u32 ms)
{
    Sleep (ms);
}
