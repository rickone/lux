#include "variant_int.h"
#include <algorithm> // std::min
#include "error.h"

uint32_t zigzag_encode32(int32_t value)
{
    return (uint32_t)((value << 1) ^ (value >> 31));
}

int32_t zigzag_decode32(uint32_t value)
{
    return (int32_t)((value >> 1) ^ (-(int32_t)(value & 1)));
}

uint64_t zigzag_encode64(int64_t value)
{
    return (uint64_t)((value << 1) ^ (value >> 63));
}

int64_t zigzag_decode64(uint64_t value)
{
    return (int64_t)((value >> 1) ^ (-(int64_t)(value & 1)));
}

void varint_pack(std::string &str, uint64_t var_int)
{
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

uint64_t varint_unpack(const std::string &str, size_t pos, size_t *read_len)
{
    size_t sz = str.size();
    runtime_assert(pos < sz, "pos overflow");

    size_t parse_len = std::min((size_t)5u, sz - pos);
    size_t var_len = 0;
    for (size_t i = 0; i < parse_len; ++i)
    {
        if (!((uint8_t)str.at(pos + i) & 0x80))
        {
            var_len = i + 1;
            break;
        }
    }
    runtime_assert(var_len > 0, "illegal varint32");

    uint64_t var_int = 0;
    if (var_len == 1)
    {
        var_int = (uint32_t)(uint8_t)str.at(pos);
    }
    else
    {
        for (size_t i = var_len - 1; i > 0; --i)
            var_int = var_int << 7 | ((uint8_t)str.at(pos + i) & 0x7f);
        var_int = var_int << 6 | ((uint8_t)str.at(pos) & 0x3f);
    }

    *read_len = var_len;
    return var_int;
}
