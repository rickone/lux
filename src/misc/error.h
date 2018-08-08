#pragma once

#include <string>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#define __FUNC__ __FUNCTION__
#else
#define __FUNC__ __PRETTY_FUNCTION__
#endif

std::string make_error_info(const char *fmt, ...);

#define throw_error(Class, fmt, ...) throw Class(make_error_info("in %s at %s:%d " fmt, __FUNC__, __FILE__, __LINE__,## __VA_ARGS__))
#define throw_lua_error(L) throw_error(std::runtime_error, "[Lua] %s", lua_tostring(L, -1))

#define throw_system_error(errcode, fmt, ...) \
    do \
    { \
        int __err = (int)(errcode); \
        throw std::system_error(std::error_code(__err, std::system_category()), \
            make_error_info("in %s at %s:%d errno(%d) " fmt, __FUNC__, __FILE__, __LINE__, __err,## __VA_ARGS__)); \
    } while (false)

#define logic_assert(Condition, fmt, ...) \
    if (!(Condition)) \
        throw_error(std::logic_error, fmt,## __VA_ARGS__)

#define runtime_assert(Condition, fmt, ...) \
    if (!(Condition)) \
        throw_error(std::runtime_error, fmt,## __VA_ARGS__)
