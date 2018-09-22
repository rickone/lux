#include "error.h"
#include <cstdarg> // va_list
#ifdef _WIN32
#include <windows.h>
#include <codecvt>
#endif

std::string make_error_info(const char *fmt, ...)
{
    static char buffer[512];

    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    return std::string(buffer);
}

#ifdef _WIN32
std::string win32_category::message(int condition) const
{
    LPVOID lpMsgBuf = NULL;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        (DWORD)condition,
        LANG_USER_DEFAULT,
        (LPWSTR)&lpMsgBuf,
        0, NULL);

    std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
    std::string result = conv.to_bytes((wchar_t *)lpMsgBuf);

    LocalFree(lpMsgBuf);
    return result;
}
#endif
