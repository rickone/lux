#include "lux_proto.h"

size_t lux_proto_pack(Buffer *buffer, lua_State *L, int index)
{
    int type = lua_type(L, index);
    if (index < 0)
        index = lua_gettop(L) + index + 1;

    switch (type)
    {
        case LUA_TNIL:
            return lux_proto_pack_nil(buffer);

        case LUA_TNUMBER:
            if (lua_isinteger(L, index))
            {
                long value = (long)lua_tointeger(L, index);
                return lux_proto_pack_int(buffer, value);
            }
            else
            {
                double value = lua_tonumber(L, index);
                return lux_proto_pack_number(L, value);
            }

        case LUA_TBOOLEAN:
            bool value = (bool)lua_toboolean(L, index);
            return lux_proto_pack_bool(buffer, value);

        case LUA_TSTRING:
            size_t len = 0;
            const char *data = lua_tolstring(L, index, &len);
            return lux_proto_pack_string(buffer, data, len);

        case LUA_TTABLE:
            size_t pack_len = lux_proto_pack_table_begin(buffer);

            lua_pushnil(L);
            while (lua_next(L, index))
            {
                pack_len += lux_proto_pack(buffer, L, -2);
                pack_len += lux_proto_pack(buffer, L, -1);
            }
            pack_len += lux_proto_pack_table_end(buffer);
            return pack_len;
    }
    return 0;
}

size_t lux_proto_pack_args(Buffer *buffer, lua_State *L, int n)
{
    size_t pack_len = lux_proto_pack_args_begin(buffer);

    int top = lua_gettop(L);
    if (n > top)
        return luaL_error(L, "lux_proto_pack_args error: n(%d) > top(%d)", n, top);

    for (int i = top - n + 1; i <= top; ++i)
    {
        pack_len += lux_proto_pack(buffer, L, i);
    }

    pack_len += lux_proto_pack_args_end(buffer);
    return pack_len;
}

size_t lux_proto_pack_nil(Buffer *buffer)
{
    buffer->push("\xC0", 1);
    return 1;
}

size_t lux_proto_pack_bool(Buffer *buffer, bool value)
{
    if (value)
        buffer->push("\xC2", 1);
    else
        buffer->push("\xC1", 1);
    return 1;
}

size_t lux_proto_pack_int(Buffer *buffer, long value)
{
    variant_int_write(buffer, value);
}

size_t lux_proto_pack_number(Buffer *buffer, double value)
{

}

size_t lux_proto_pack_string(Buffer *buffer, const char *data, size_t len)
{

}

size_t lux_proto_pack_table_begin(Buffer *buffer)
{

}

size_t lux_proto_pack_table_end(Buffer *buffer)
{

}

size_t lux_proto_pack_args_begin(Buffer *buffer)
{

}

size_t lux_proto_pack_args_end(Buffer *buffer)
{

}

int lux_proto_unpack(lua_State *L, Buffer *buffer, size_t len)
{

}

void lux_proto_unpack_object(LuxProto *lux_proto, Buffer *buffer, size_t len)
{

}
