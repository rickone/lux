#include "redis_proto.h"
#include "error.h"

using namespace lux;

void RedisProto::new_class(lua_State *L)
{
    lua_new_class(L, RedisProto);

    lua_newtable(L);
    {
        lua_method(L, clear);
        lua_std_method(L, pack);
        lua_std_method(L, unpack);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_property(L, str);
        lua_property(L, pos);
    }
    lua_setfield(L, -2, "__property");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_redis_proto", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<RedisProto> RedisProto::create()
{
    return std::make_shared<RedisProto>();
}

void RedisProto::clear()
{
    _str.clear();
    _pos = 0;
}

void RedisProto::pack(std::nullptr_t ptr)
{
    redis_pack_bulk_string(_str, nullptr, 0);
}

void RedisProto::pack(const RedisObject &obj)
{
    obj.serialize(_str);
}

void RedisProto::pack(const std::string &value)
{
    redis_pack_simple_string(_str, value);
}

void RedisProto::pack_bulk_string(const char *data, size_t len)
{
    redis_pack_bulk_string(_str, data, len);
}

std::shared_ptr<RedisObject> RedisProto::unpack()
{
    std::string::size_type crlf = _str.find("\r\n", _pos + 1, 2);
    runtime_assert(crlf != std::string::npos, "cant find CRLF");

    char header = _str.at(_pos);
    ++_pos;

    std::shared_ptr<RedisObject> result;
    switch (header)
    {
        case REDIS_HEADER_SIMPLE_STRING:
        {
            result = std::make_shared<RedisSimpleString>(_str, _pos, crlf - _pos);
            _pos = crlf + 2;
            break;
        }

        case REDIS_HEADER_ERROR:
        {
            result = std::make_shared<RedisError>(_str, _pos, crlf - _pos);
            _pos = crlf + 2;
            break;
        }

        case REDIS_HEADER_INTEGER:
        {
            result = std::make_shared< RedisInteger<long long> >(std::stoll(std::string(_str, _pos, crlf - _pos)));
            _pos = crlf + 2;
            break;
        }

        case REDIS_HEADER_BULK_STRING:
        {
            long val = std::stol(std::string(_str, _pos, crlf - _pos));
            _pos = crlf + 2;

            if (val < 0)
            {
                result = std::make_shared<RedisNull>(REDIS_HEADER_BULK_STRING);
                break;
            }

            size_t len = (size_t)val;
            runtime_assert(_pos + len + 2 <= _str.size(), "bulk string overflow");
            result = std::make_shared<RedisBulkString>(_str, _pos, len);
            _pos += len + 2;
            break;
        }

        case REDIS_HEADER_ARRAY:
        {
            int count = std::stoi(std::string(_str, _pos, crlf - _pos));
            _pos = crlf + 2;

            if (count < 0)
            {
                result = std::make_shared<RedisNull>(REDIS_HEADER_ARRAY);
                break;
            }

            auto array_obj = std::make_shared<RedisArray>();
            for (int i = 0; i < count; ++i)
            {
                auto obj = unpack();
                array_obj->push(obj);
            }
            result = array_obj;
            break;
        }

        default:
            throw_error(std::runtime_error, "header=0x%02X", header);
    }

    return result;
}

int RedisProto::lua_pack(lua_State *L)
{
    int top = lua_gettop(L);
    for (int i = 1; i <= top; ++i)
    {
        pack_lua_object(L, i);
    }
    return 0;
}

int RedisProto::lua_unpack(lua_State *L)
{
    int nresults = 0;
    while (_pos < _str.size())
    {
        nresults += unpack_lua_object(L);
    }
    return nresults;
}

void RedisProto::pack_lua_object(lua_State *L, int index)
{
    if (index < 0)
        index = lua_gettop(L) + index + 1;

    int type = lua_type(L, index);
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
                auto value = std::to_string(lua_tonumber(L, index));
                pack_bulk_string(value.data(), value.size());
            }
            break;
        }

        case LUA_TBOOLEAN:
        {
            int value = lua_toboolean(L, index);
            pack(value);
            break;
        }

        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *data = lua_tolstring(L, index, &len);
            pack_bulk_string(data, len);
            break;
        }

        case LUA_TTABLE:
        {
            RedisProto body;

            lua_Integer n = 0;
            while (true)
            {
                int t = lua_geti(L, index, ++n);
                if (t == LUA_TNIL)
                    break;

                body.pack_lua_object(L, -1);
                lua_pop(L, 1);
            }

            redis_pack_array(_str, n - 1, body.str().data(), body.str().size());
            break;
        }

        default:
            luaL_error(L, "redis-proto.pack error: type=%d", type);
    }
}

int RedisProto::unpack_lua_object(lua_State *L)
{
    std::string::size_type crlf = _str.find("\r\n", _pos + 1, 2);
    runtime_assert(crlf != std::string::npos, "cant find CRLF");

    char header = _str.at(_pos);
    ++_pos;

    switch (header)
    {
        case REDIS_HEADER_SIMPLE_STRING:
        {
            lua_pushlstring(L, _str.data() + _pos, crlf - _pos);
            _pos = crlf + 2;
            break;
        }

        case REDIS_HEADER_ERROR:
        {
            lua_pushlstring(L, _str.data() + _pos, crlf - _pos);
            _pos = crlf + 2;
            break;
        }

        case REDIS_HEADER_INTEGER:
        {
            long long val = std::stoll(std::string(_str, _pos, crlf - _pos));
            lua_pushinteger(L, (lua_Integer)val);
            _pos = crlf + 2;
            break;
        }

        case REDIS_HEADER_BULK_STRING:
        {
            long val = std::stol(std::string(_str, _pos, crlf - _pos));
            _pos = crlf + 2;

            if (val < 0)
            {
                lua_pushnil(L);
                break;
            }

            size_t len = (size_t)val;
            runtime_assert(_pos + len + 2 <= _str.size(), "bulk string overflow");
            lua_pushlstring(L, _str.data() + _pos, len);
            _pos += len + 2;
            break;
        }

        case REDIS_HEADER_ARRAY:
        {
            int count = std::stoi(std::string(_str, _pos, crlf - _pos));
            _pos = crlf + 2;

            if (count < 0)
            {
                lua_pushnil(L);
                break;
            }

            lua_createtable(L, count, 0);
            for (int i = 0; i < count; ++i)
            {
                unpack_lua_object(L);
                lua_rawseti(L, -2, (lua_Integer)(i + 1));
            }
            break;
        }

        default:
            return luaL_error(L, "redis-proto.unpack error: header=0x%02X", header);
    }
    return 1;
}
