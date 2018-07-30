#include "variant_int.h"
#include <algorithm>
#include "buffer.h"

inline unsigned long zigzag_encode(long value)
{
    return (value << 1) ^ (value >> (sizeof(value) * 8 - 1));
}

inline long zigzag_decode(unsigned long value)
{
    return (value >> 1) ^ -(value & 1);
}

size_t variant_int_write(Buffer *buffer, long value)
{
    unsigned long var_int = zigzag_encode(value);

    if (var_int <= 0x7f)
    {
        uint8_t byte = (uint8_t)var_int;
        buffer->push((const char *)&byte, sizeof(byte));
        return sizeof(byte);
    }

    size_t write_len = 0;
    uint8_t header = (uint8_t)(var_int & 0x3f) | 0x80;
    var_int >>= 6;

    buffer->push((const char *)&header, sizeof(header));
    write_len += sizeof(header);

    while (true)
    {
        uint8_t byte = var_int & 0x7f;
        var_int >>= 7;

        if (!var_int)
        {
            buffer->push((const char *)&byte, sizeof(byte));
            write_len += sizeof(byte);
            break;
        }

        byte |= 0x80;
        buffer->push((const char *)&byte, sizeof(byte));
        write_len += sizeof(byte);
    }

    return write_len;
}

size_t variant_int_read(Buffer *buffer, long *result)
{
    size_t sz = buffer->size();
    if (sz == 0)
        return 0;

    size_t parse_len = std::min((size_t)10ul, sz);
    size_t var_len = 0;
    for (size_t i = 0; i < parse_len; ++i)
    {
        if (!(*(uint8_t *)buffer->data(i) & 0x80))
        {
            var_len = i + 1;
            break;
        }
    }

    if (var_len == 0)
    {
        if (parse_len > 10)
        {
            std::string str = buffer->dump();
            buffer->clear();
            throw_error(VariantIntError, "buffer dump: %s", str.c_str());
        }

        return 0;
    }

    unsigned long var_int = 0;
    if (var_len == 1)
    {
        var_int = (unsigned long)*(uint8_t *)buffer->data(0);
    }
    else
    {
        for (size_t i = var_len - 1; i > 0; --i)
            var_int = var_int << 7 | (*(uint8_t *)buffer->data(i) & 0x7f);
        var_int = var_int << 6 | (*(uint8_t *)buffer->data(0) & 0x3f);
    }

    *result = zigzag_decode(var_int);
    return var_len;
}
