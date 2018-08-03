#include "variant_int.h"
#include <algorithm>
#include "error.h"

inline unsigned long zigzag_encode(long value)
{
    return (value << 1) ^ (value >> (sizeof(value) * 8 - 1));
}

inline long zigzag_decode(unsigned long value)
{
    return (value >> 1) ^ -(value & 1);
}

void variant_int_write(std::string &str, long value)
{
    unsigned long var_int = zigzag_encode(value);

    if (var_int <= 0x7f)
    {
        uint8_t byte = (uint8_t)var_int;
        str.append((const char *)&byte, sizeof(byte));
        return;
    }

    uint8_t header = (uint8_t)(var_int & 0x3f) | 0x80;
    var_int >>= 6;

    str.append((const char *)&header, sizeof(header));

    while (true)
    {
        uint8_t byte = var_int & 0x7f;
        var_int >>= 7;

        if (!var_int)
        {
            str.append((const char *)&byte, sizeof(byte));
            break;
        }

        byte |= 0x80;
        str.append((const char *)&byte, sizeof(byte));
    }
}

long variant_int_read(const std::string &str, size_t pos, size_t *read_len)
{
    size_t sz = str.size();
    runtime_assert(pos < sz, "string pos overflow");

    size_t parse_len = std::min((size_t)10ul, sz - pos);
    size_t var_len = 0;
    for (size_t i = 0; i < parse_len; ++i)
    {
        if (!((uint8_t)str.at(pos + i) & 0x80))
        {
            var_len = i + 1;
            break;
        }
    }
    runtime_assert(var_len > 0, "string is illegal variant-int");

    unsigned long var_int = 0;
    if (var_len == 1)
    {
        var_int = (unsigned long)(uint8_t)str.at(pos);
    }
    else
    {
        for (size_t i = var_len - 1; i > 0; --i)
            var_int = var_int << 7 | ((uint8_t)str.at(pos + i) & 0x7f);
        var_int = var_int << 6 | ((uint8_t)str.at(pos) & 0x3f);
    }

    *read_len = var_len;
    return zigzag_decode(var_int);
}
