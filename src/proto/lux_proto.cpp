#include "lux_proto.h"
#include <cstdio>

LuxProto::LuxProto(const LuxProto &other) : _str(other._str)
{
}

LuxProto & LuxProto::operator =(const LuxProto &other)
{
    _str = other._str;
    _pos = 0;
    _luxp_objs.clear();
    return *this;
}

void LuxProto::new_class(lua_State *L)
{
    lua_new_class(L, LuxProto);

    lua_newtable(L);
    {
        lua_method(L, clear);
        lua_method(L, dump);
        lua_std_method(L, pack);
        lua_std_method(L, packlist);
        lua_std_method(L, unpack);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_property(L, pos);
        lua_property(L, str);
    }
    lua_setfield(L, -2, "__property");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_lux_proto", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<LuxProto> LuxProto::create()
{
    return std::make_shared<LuxProto>();
}

void LuxProto::clear()
{
    _str.clear();
    _pos = 0;
    _luxp_objs.clear();
}

std::string LuxProto::dump()
{
    std::string result = "result=(";
    for (char c : _str)
    {
        static char temp[32];
        sprintf(temp, "%02x,", (unsigned char)c);
        result += temp;
    }
    result += ")";
    return result;
}

int LuxProto::lua_pack(lua_State *L)
{
    int top = lua_gettop(L);
    for (int i = 1; i <= top; ++i)
    {
        pack_lua_object(L, i);
    }
    return 0;
}

int LuxProto::lua_packlist(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    _str.push_back((char)LUX_HEADER_LIST);

    int top = lua_gettop(L);
    size_t n = 0;
    for (int i = 1; ; ++i)
    {
        int type = lua_rawgeti(L, 1, i);
        if (type == LUA_TNIL)
            break;

        ++n;
    }
    lua_settop(L, top);
    varint_pack(_str, n);

    for (int i = 1; i <= (int)n; ++i)
    {
        lua_rawgeti(L, 1, i);
        pack_lua_object(L, -1);
        lua_pop(L, 1);
    }
    return 0;
}

int LuxProto::lua_unpack(lua_State *L)
{
    int nresults = 0;
    while (_pos < _str.size())
    {
        nresults += unpack_lua_object(L);
    }
    return nresults;
}

void LuxProto::pack_lua_object(lua_State *L, int index)
{
    int type = lua_type(L, index);
    if (index < 0)
        index = lua_gettop(L) + index + 1;

    switch (type)
    {
        case LUA_TNIL:
        {
            pack(nullptr);
            break;
        }

        case LUA_TNUMBER:
        {
            if (lua_isinteger(L, index))
            {
                long value = (long)lua_tointeger(L, index);
                pack(value);
            }
            else
            {
                double value = lua_tonumber(L, index);
                pack(value);
            }
            break;
        }

        case LUA_TBOOLEAN:
        {
            bool value = (0 != lua_toboolean(L, index));
            pack(value);
            break;
        }

        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *data = lua_tolstring(L, index, &len);
            lux_pack_string(_str, data, len);
            break;
        }

        case LUA_TTABLE:
        {
            _str.push_back((char)LUX_HEADER_DICT);

            size_t n = 0;
            lua_pushnil(L);
            while (lua_next(L, index))
            {
                ++n;
                lua_pop(L, 1);
            }
            varint_pack(_str, n);

            lua_pushnil(L);
            while (lua_next(L, index))
            {
                pack_lua_object(L, -2);
                pack_lua_object(L, -1);
                lua_pop(L, 1);
            }
            break;
        }

        default:
            luaL_error(L, "lux-proto.pack error: type=%d", type);
    }
}

int LuxProto::unpack_lua_object(lua_State *L)
{
    uint8_t header = (uint8_t)_str.at(_pos);
    if (is_varint_header(header))
    {
        long value = unpack<long>();
        lua_pushinteger(L, value);
        return 1;
    }

    switch (header)
    {
        case LUX_HEADER_NULL:
        {
            lua_pushnil(L);
            ++_pos;
            break;
        }

        case LUX_HEADER_FALSE:
        {
            lua_pushboolean(L, false);
            ++_pos;
            break;
        }

        case LUX_HEADER_TRUE:
        {
            lua_pushboolean(L, true);
            ++_pos;
            break;
        }

        case LUX_HEADER_FLOAT:
        {
            float value = unpack<float>();
            lua_pushnumber(L, value);
            break;
        }

        case LUX_HEADER_DOUBLE:
        {
            double value = unpack<double>();
            lua_pushnumber(L, value);
            break;
        }

        case LUX_HEADER_STRING:
        {
            size_t used_len = 0;
            size_t len = 0;
            const char *data = lux_unpack_string(_str, _pos, &len, &used_len);
            lua_pushlstring(L, data, len);
            _pos += used_len;
            break;
        }

        case LUX_HEADER_LIST:
        {
            size_t lst_len_len = 0;
            size_t lst_len = varint_unpack(_str, _pos + 1, &lst_len_len);
            _pos += 1 + lst_len_len;

            lua_createtable(L, (int)lst_len, 0);
            for (size_t i = 0; i < lst_len; ++i)
            {
                unpack_lua_object(L);
                lua_rawseti(L, -2, (lua_Integer)(i + 1));
            }
            break;
        }

        case LUX_HEADER_DICT:
        {
            size_t dict_len_len = 0;
            size_t dict_len = varint_unpack(_str, _pos + 1, &dict_len_len);
            _pos += 1 + dict_len_len;

            lua_newtable(L);
            for (size_t i = 0; i < dict_len; ++i)
            {
                unpack_lua_object(L);
                unpack_lua_object(L);
                lua_rawset(L, -3);
            }
            break;
        }

        default:
            return luaL_error(L, "lux-proto.unpack error: header=0x%02X", header);
    }
    return 1;
}
