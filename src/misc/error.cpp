#include "error.h"
#include <cstdarg> // va_list

std::string make_error_info(const char *fmt, ...)
{
    static char buffer[512];

    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    return std::string(buffer);
}
