#include "variant_int.h"
#include <algorithm> // std::min
#include "error.h"

//using namespace lux;

void lux::varint_pack(std::string &str, uint64_t var_int)
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

uint64_t lux::varint_unpack(const std::string &str, size_t pos, size_t *used_len)
{
    uint8_t header = (uint8_t)str.at(pos);
    runtime_assert(is_varint_header(header), "varint header illegal");

    size_t sz = str.size();
    size_t parse_len = std::min((size_t)10u, sz - pos);
    size_t var_len = 0;
    for (size_t i = 0; i < parse_len; ++i)
    {
        if (!((uint8_t)str.at(pos + i) & 0x80))
        {
            var_len = i + 1;
            break;
        }
    }
    runtime_assert(var_len > 0, "varint length illegal");

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

    *used_len = var_len;
    return var_int;
}
