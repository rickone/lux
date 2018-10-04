#include "lua_proto.h"
#include "variant_int.h"
#include "error.h"

inline void lua_proto_pack_nil(std::string &str)
{
    str.append("\xC0", 1);
}

inline void lua_proto_pack_bool(std::string &str, bool value)
{
    if (value)
        str.append("\xC2", 1);
    else
        str.append("\xC1", 1);
}

inline void lua_proto_pack_int(std::string &str, long value)
{
    varint_pack(str, zigzag_encode32(value));
}

inline void lua_proto_pack_number(std::string &str, double value)
{
    str.append("\xC3", 1);
    str.append((const char *)&value, sizeof(value));
}

inline void lua_proto_pack_string(std::string &str, const char *data, size_t len)
{
    str.append("\xC4", 1);
    varint_pack(str, len);
    str.append(data, len);
}

inline void lua_proto_pack_new_table(std::string &str)
{
    str.append("\xC5", 1);
}

void lua_proto_pack(std::string &str, lua_State *L, int index)
{
    int type = lua_type(L, index);
    if (index < 0)
        index = lua_gettop(L) + index + 1;

    switch (type)
    {
        case LUA_TNIL:
        {
            lua_proto_pack_nil(str);
            break;
        }

        case LUA_TNUMBER:
        {
            if (lua_isinteger(L, index))
            {
                long value = (long)lua_tointeger(L, index);
                lua_proto_pack_int(str, value);
            }
            else
            {
                double value = lua_tonumber(L, index);
                lua_proto_pack_number(str, value);
            }
            break;
        }

        case LUA_TBOOLEAN:
        {
            int value = lua_toboolean(L, index);
            lua_proto_pack_bool(str, value != 0);
            break;
        }

        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *data = lua_tolstring(L, index, &len);
            lua_proto_pack_string(str, data, len);
            break;
        }

        case LUA_TTABLE:
        {
            lua_proto_pack_new_table(str);
            lua_pushnil(L);
            while (lua_next(L, index))
            {
                lua_proto_pack(str, L, -2);
                lua_proto_pack(str, L, -1);

                lua_pop(L, 1);
            }
            lua_proto_pack_nil(str);
            break;
        }

        default:
            throw_error(std::runtime_error, "type(%d)", type);
    }
}

void lua_proto_pack_args(std::string &str, lua_State *L, int n)
{
    runtime_assert(n > 0, "n(%d)", n);

    int top = lua_gettop(L);
    runtime_assert(n <= top, "n(%d) top(%d)", n, top);
    
    for (int i = top - n + 1; i <= top; ++i)
    {
        lua_proto_pack(str, L, i);
    }
}

inline size_t lua_proto_unpack_int(lua_State *L, const std::string &str, size_t pos)
{
    size_t var_len = 0;
    long value = zigzag_decode32(varint_unpack(str, pos, &var_len));

    lua_pushinteger(L, value);
    return var_len;
}

inline size_t lua_proto_unpack_number(lua_State *L, const std::string &str, size_t pos)
{
    double value = 0;
#ifdef _WIN32
    str._Copy_s((char *)&value, sizeof(value), pos);
#else
    str.copy((char *)&value, sizeof(value), pos);
#endif

    lua_pushnumber(L, value);
    return sizeof(value);
}

inline size_t lua_proto_unpack_string(lua_State *L, const std::string &str, size_t pos)
{
    size_t var_len = 0;
    size_t str_len = varint_unpack(str, pos, &var_len);
    lua_pushlstring(L, str.data() + pos + var_len, str_len);
    return var_len + str_len;
}

size_t lua_proto_unpack(lua_State *L, const std::string &str, size_t pos)
{
    uint8_t header = (uint8_t)str.at(pos);

    uint8_t tag = (header & 0xC0);
    if (tag != 0xC0)
        return lua_proto_unpack_int(L, str, pos);

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
            unpack_len += lua_proto_unpack_number(L, str, pos + 1);
            break;
        }

        case 0xC4:
        {
            unpack_len += lua_proto_unpack_string(L, str, pos + 1);
            break;
        }

        case 0xC5:
        {
            lua_newtable(L);

            while (true)
            {
                unpack_len += lua_proto_unpack(L, str, pos + unpack_len);

                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    break;
                }

                unpack_len += lua_proto_unpack(L, str, pos + unpack_len);

                lua_settable(L, -3);
            }
            break;
        }

        default:
            throw_error(std::runtime_error, "header(0x%02X)", header);
    }

    return unpack_len;
}

int lua_proto_unpack_args(lua_State *L, const std::string &str)
{
    size_t sz = str.size();
    size_t pos = 0;
    int n = 0;

    while (pos < sz)
    {
        pos += lua_proto_unpack(L, str, pos);
        ++n;
    }

    return n;
}

int luap_pack(lua_State *L)
{
    std::string str;
    int n = lua_gettop(L);

    lua_proto_pack_args(str, L, n);

    lua_pushlstring(L, str.data(), str.size());
    return 1;
}

int luap_unpack(lua_State *L)
{
    size_t len = 0;
    const char *data = lua_tolstring(L, 1, &len);
    std::string str(data, len);

    return lua_proto_unpack_args(L, str);
}
