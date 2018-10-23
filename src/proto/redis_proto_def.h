#pragma once

#include <cstddef> // size_t
#include <string>

namespace lux {

#define REDIS_HEADER_SIMPLE_STRING  ('+')
#define REDIS_HEADER_ERROR          ('-')
#define REDIS_HEADER_INTEGER        (':')
#define REDIS_HEADER_BULK_STRING    ('$')
#define REDIS_HEADER_ARRAY          ('*')

// simple string
inline void redis_pack_simple_string(std::string &str, const std::string &value)
{
    str.push_back(REDIS_HEADER_SIMPLE_STRING);
    str.append(value);
    str.append("\r\n", 2);
}

// error
inline void redis_pack_error(std::string &str, const std::string &value)
{
    str.push_back(REDIS_HEADER_ERROR);
    str.append(value);
    str.append("\r\n", 2);
}

// integer
template<typename T>
inline void redis_pack_integer(std::string &str, T value)
{
    str.push_back(REDIS_HEADER_INTEGER);
    str.append(std::to_string(value));
    str.append("\r\n", 2);
}

// bulk string
inline void redis_pack_bulk_string(std::string &str, const char *data, size_t len)
{
    str.push_back(REDIS_HEADER_BULK_STRING);
    if (data == nullptr)
    {
        str.append("-1\r\n", 4); // null bulk string
        return;
    }

    str.append(std::to_string(len));
    str.append("\r\n", 2);
    str.append(data, len);
    str.append("\r\n", 2);
}

// array
inline void redis_pack_array(std::string &str, size_t count, const char *data, size_t len)
{
    str.push_back(REDIS_HEADER_ARRAY);
    if (data == nullptr)
    {
        str.append("-1\r\n", 4); // null array
        return;
    }

    str.append(std::to_string(count));
    str.append("\r\n", 2);
    str.append(data, len);
}

} // lux
