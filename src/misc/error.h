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

#define throw_system_error(code, category, fmt, ...) \
    do \
    { \
        int __err = (int)(code); \
        throw std::system_error(std::error_code(__err, category), \
            make_error_info("in %s at %s:%d errno(%d) " fmt, __FUNC__, __FILE__, __LINE__, __err,## __VA_ARGS__)); \
    } while (false)

#define throw_unix_error(...) throw_system_error(errno, std::system_category(),## __VA_ARGS__)

#define logic_assert(Condition, fmt, ...) \
    if (!(Condition)) \
        throw_error(std::logic_error, fmt,## __VA_ARGS__)

#define runtime_assert(Condition, fmt, ...) \
    if (!(Condition)) \
        throw_error(std::runtime_error, fmt,## __VA_ARGS__)

#ifdef _WIN32

struct win32_category : std::error_category
{
    virtual const char *name() const noexcept override { return "win32_category"; }

    virtual std::string message(int condition) const override;
};

#define throw_win32_error(...) throw_system_error(GetLastError(), win32_category(),## __VA_ARGS__)
#define throw_win32_wsa_error(...) throw_system_error(WSAGetLastError(), win32_category(),## __VA_ARGS__)

#endif
