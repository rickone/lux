#include "lux_proto.h"
#include "variant_int.h"

inline size_t lux_proto_pack_nil(Buffer *buffer)
{
    buffer->push("\xC0", 1);
    return 1;
}

inline size_t lux_proto_pack_bool(Buffer *buffer, bool value)
{
    if (value)
        buffer->push("\xC2", 1);
    else
        buffer->push("\xC1", 1);
    return 1;
}

inline size_t lux_proto_pack_int(Buffer *buffer, long value)
{
    return variant_int_write(buffer, value);
}

inline size_t lux_proto_pack_number(Buffer *buffer, double value)
{
    buffer->push("\xC3", 1);
    buffer->push((const char *)&value, sizeof(value));
    return 1 + sizeof(value);
}

inline size_t lux_proto_pack_string(Buffer *buffer, const char *data, size_t len)
{
    buffer->push("\xC4", 1);
    size_t var_len = variant_int_write(buffer, (long)len);
    buffer->push(data, len);
    return 1 + var_len + len;
}

inline size_t lux_proto_pack_new_table(Buffer *buffer)
{
    buffer->push("\xC5", 1);
    return 1;
}

size_t lux_proto_pack(Buffer *buffer, lua_State *L, int index)
{
    int type = lua_type(L, index);
    if (index < 0)
        index = lua_gettop(L) + index + 1;

    switch (type)
    {
        case LUA_TNIL:
        {
            return lux_proto_pack_nil(buffer);
        }

        case LUA_TNUMBER:
        {
            if (lua_isinteger(L, index))
            {
                long value = (long)lua_tointeger(L, index);
                return lux_proto_pack_int(buffer, value);
            }
            else
            {
                double value = lua_tonumber(L, index);
                return lux_proto_pack_number(buffer, value);
            }
        }

        case LUA_TBOOLEAN:
        {
            bool value = (bool)lua_toboolean(L, index);
            return lux_proto_pack_bool(buffer, value);
        }

        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *data = lua_tolstring(L, index, &len);
            return lux_proto_pack_string(buffer, data, len);
        }

        case LUA_TTABLE:
        {
            size_t pack_len = lux_proto_pack_new_table(buffer);

            lua_pushnil(L);
            while (lua_next(L, index))
            {
                pack_len += lux_proto_pack(buffer, L, -2);
                pack_len += lux_proto_pack(buffer, L, -1);
            }
            pack_len += lux_proto_pack_nil(buffer);

            return pack_len;
        }
    }
    return 0;
}

size_t lux_proto_pack_args(Buffer *buffer, lua_State *L, int n)
{
    runtime_assert(n > 0, "n(%d)", n);

    int top = lua_gettop(L);
    runtime_assert(n <= top, "n(%d) top(%d)", n, top);
    
    size_t pack_len = 0;
    for (int i = top - n + 1; i <= top; ++i)
    {
        pack_len += lux_proto_pack(buffer, L, i);
    }

    return pack_len;
}

inline size_t lux_proto_unpack_int(lua_State *L, Buffer *buffer, size_t len)
{
    long value = 0;
    size_t var_len = variant_int_read(buffer, &value);
    runtime_assert(len >= var_len, "len(%u) var_len(%u)", len, var_len);

    lua_pushinteger(L, value);
    buffer->pop(nullptr, var_len);
    return var_len;
}

inline size_t lux_proto_unpack_number(lua_State *L, Buffer *buffer, size_t len)
{
    runtime_assert(len >= sizeof(double), "len(%u)", len);

    double value = 0;
    buffer->pop((char *)&value, sizeof(double));

    lua_pushnumber(L, value);
    return sizeof(double);
}

inline size_t lux_proto_unpack_string(lua_State *L, Buffer *buffer, size_t len)
{
    long value = 0;
    size_t var_len = variant_int_read(buffer, &value);
    runtime_assert(value >= 0, "value(%ld)", value);

    size_t str_len = (size_t)value;
    size_t unpack_len = var_len + str_len;
    runtime_assert(len >= unpack_len, "len(%u) var_len(%u) str_len(%u)", len, var_len, str_len);

    std::string str = buffer->get_string(var_len, str_len);

    lua_pushlstring(L, str.data(), str.size());
    buffer->pop(nullptr, unpack_len);
    return unpack_len;
}

size_t lux_proto_unpack(lua_State *L, Buffer *buffer, size_t len)
{
    runtime_assert(len > 0, "len(%u)", len);

    uint8_t header = *(uint8_t *)buffer->data(0);

    uint8_t tag = (header & 0xC0);
    if (tag != 0xC0)
        return lux_proto_unpack_int(L, buffer, len);

    buffer->pop(nullptr, 1);
    size_t unpack_len = 1;

    switch (header)
    {
        case 0xC0:
        {
            lua_pushnil(L);
            break;
        }

        case 0xC1:
        {
            lua_pushboolean(L, false);
            break;
        }

        case 0xC2:
        {
            lua_pushboolean(L, true);
            break;
        }

        case 0xC3:
        {
            unpack_len += lux_proto_unpack_number(L, buffer, len - 1);
            break;
        }

        case 0xC4:
        {
            unpack_len += lux_proto_unpack_string(L, buffer, len - 1);
            break;
        }

        case 0xC5:
        {
            lua_newtable(L);

            while (true)
            {
                unpack_len += lux_proto_unpack(L, buffer, len - unpack_len);

                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    break;
                }

                unpack_len += lux_proto_unpack(L, buffer, len - unpack_len);

                lua_settable(L, -3);
            }
            break;
        }

        default:
            throw_error(std::runtime_error, "header(0x%02X)", header);
    }

    return unpack_len;
}

int lux_proto_unpack_args(lua_State *L, Buffer *buffer, size_t len)
{
    size_t unpack_len = 0;
    int n = 0;

    while (unpack_len < len)
    {
        unpack_len += lux_proto_unpack(L, buffer, len - unpack_len);
        ++n;
    }

    return n;
}
