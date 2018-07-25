#include "resp_object.h"
#include <cassert>
#include <algorithm>
#include "lua_port.h"
#include "buffer.h"

void RespObject::new_class(lua_State *L)
{
    lua_new_class(L, RespObject);

    lua_newtable(L);
    {
        lua_method(L, clear);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_method(L, create);
    }
    lua_setglobal(L, "resp");
}

std::shared_ptr<RespObject> RespObject::create()
{
    return std::shared_ptr<RespObject>(new RespObject());
}

void RespObject::clear()
{
    _type = kRespType_Null;
    _value.clear();
    _array.clear();
    _phase = 0;
    _value_left_length = 0;
}

void RespObject::set_type(int type)
{
    _type = type;
    _value = "";
}

void RespObject::set_value(int type, const std::string &value)
{
    _type = type;
    _value = value;
}

void RespObject::set_value(int type, const char *data, size_t len)
{
    _type = type;
    _value.assign(data, len);
}

void RespObject::add_array_member(const RespObject &obj)
{
    if (_type != kRespType_Array)
        throw_error(ProtoError, "[Resp] _type = %d", _type);

    _array.push_back(obj);
}

bool RespObject::is_error()
{
    if (_type == kRespType_Error)
        return true;

    if (_type == kRespType_Array)
    {
        for (RespObject& obj : _array)
        {
            if (obj.is_error())
                return true;
        }
    }

    return false;
}

static bool find_crlf(Buffer *buffer, size_t *crlf_pos)
{
    size_t sz = buffer->size();
    if (sz < 2)
        return false;

    size_t last = sz - 1;
    for (size_t i = 0; i < last; ++i)
    {
        if (*buffer->data(i) == '\r' && *buffer->data(i + 1) == '\n')
        {
            *crlf_pos = i;
            return true;
        }
    }

    return false; 
}

int RespObject::pack(lua_State *L)
{
    int index = lua_gettop(L);
    int type = lua_type(L, index);

    switch (type)
    {
        case LUA_TNIL:
        {
            set_type(kRespType_Null);
            break;
        }

        case LUA_TNUMBER:
        {
            if (lua_isinteger(L, index))
            {
                lua_Integer value = lua_tointeger(L, index);
                set_value(kRespType_Integer, std::to_string(value));
            }
            else
            {
                double value = lua_tonumber(L, index);
                set_value(kRespType_BulkString, std::to_string(value));
            }
            break;
        }

        case LUA_TBOOLEAN:
        {
            bool value = (bool)lua_toboolean(L, index);
            set_value(kRespType_BulkString, std::to_string(value));
            break;
        }

        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *data = lua_tolstring(L, index, &len);
            set_value(kRespType_BulkString, data, len);
            break;
        }

        case LUA_TTABLE:
        {
            set_type(kRespType_Array);
            int i = 0;
            while (true)
            {
                lua_rawgeti(L, index, ++i);
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    break;
                }

                RespObject obj;
                obj.pack(L);

                _array.push_back(obj);
            }
            break;
        }

        default:
            luaL_error(L, "RespObject::pack unknown type: %d", type);
    }

    lua_pop(L, 1);
    return 1;
}

int RespObject::unpack(lua_State *L)
{
    switch (_type)
    {
        case kRespType_Null:
        {
            lua_pushnil(L);
            break;
        }

        case kRespType_SimpleString:
        case kRespType_Error:
        case kRespType_BulkString:
        {
            lua_pushlstring(L, _value.data(), _value.size());
            break;
        }

        case kRespType_Integer:
        {
            lua_pushinteger(L, std::stoi(_value));
            break;
        }

        case kRespType_Array:
        {
            lua_createtable(L, _array.size(), 0);

            lua_Integer i = 0;
            for (RespObject &obj : _array)
            {
                obj.unpack(L);
                lua_rawseti(L, -2, ++i);
            }
            break;
        }

        default:
            assert(false);
    }

    clear();
    return 1;
}

void RespObject::serialize(Buffer *buffer)
{
    switch (_type)
    {
        case kRespType_Null:
        {
            buffer->push("$-1\r\n", 5);
            break;
        }

        case kRespType_SimpleString:
        {
            buffer->push("+", 1);
            buffer->push(_value.data(), _value.size());
            buffer->push("\r\n", 2);
            break;
        }

        case kRespType_Error:
        {
            buffer->push("-", 1);
            buffer->push(_value.data(), _value.size());
            buffer->push("\r\n", 2);
            break;
        }

        case kRespType_Integer:
        {
            buffer->push(":", 1);
            buffer->push(_value.data(), _value.size());
            buffer->push("\r\n", 2);
            break;
        }

        case kRespType_BulkString:
        {
            std::string len_str = std::to_string(_value.size());

            buffer->push("$", 1);
            buffer->push(len_str.data(), len_str.size());
            buffer->push("\r\n", 2);
            buffer->push(_value.data(), _value.size());
            buffer->push("\r\n", 2);
            break;
        }

        case kRespType_Array:
        {
            std::string len_str = std::to_string(_array.size());

            buffer->push("*", 1);
            buffer->push(len_str.data(), len_str.size());
            buffer->push("\r\n", 2);

            for (RespObject &obj : _array)
            {
                obj.serialize(buffer);
            }
            break;
        }

        default:
            assert(false);
    }
}

bool RespObject::deserialize(Buffer *buffer)
{
    switch (_phase)
    {
        case 0:
            return parse_phase0(buffer);

        case 1:
            return parse_phase1(buffer);

        case 2:
            return parse_phase2(buffer);

        default:
            throw_error(ProtoError, "[Resp] _phase = %d", _phase);
    }

    return false;
}

bool RespObject::parse_phase0(Buffer *buffer)
{
    size_t sz = buffer->size();
    if (sz == 0)
        return false;

    char header = 0;
    buffer->pop(&header, 1);

    switch (header)
    {
        case '$':
        {
            _type = kRespType_BulkString;
            break;
        }

        case '*':
        {
            _type = kRespType_Array;
            break;
        }

        case '+':
        {
            _type = kRespType_SimpleString;
            break;
        }

        case '-':
        {
            _type = kRespType_Error;
            break;
        }

        case ':':
        {
            _type = kRespType_Integer;
            break;
        }

        default:
            throw_error(ProtoError, "[Resp] header = '%c'", header);
    }

    ++_phase;
    return parse_phase1(buffer);
}

bool RespObject::parse_phase1(Buffer *buffer)
{
    size_t sz = buffer->size();
    if (sz == 0)
        return false;

    size_t crlf_pos = 0;
    bool find_succ = find_crlf(buffer, &crlf_pos);
    if (find_succ)
    {
        buffer->get_string(0, _value, crlf_pos);
        buffer->pop(nullptr, crlf_pos + 2);
    }
    else
    {
        buffer->get_string(0, _value, sz);
        buffer->pop(nullptr, sz);
    }

    switch (_type)
    {
        case kRespType_BulkString:
        case kRespType_Array:
        {
            if (find_succ)
            {
                int cnt = std::stoi(_value);
                _value.resize(0);

                if (cnt < 0)
                {
                    _type = kRespType_Null;
                    return true;
                }
                
                if (cnt == 0)
                    return true;

                if (_type == kRespType_Array)
                    _array.push_back(RespObject());

                _value_left_length = (size_t)cnt;
                ++_phase;
                return parse_phase2(buffer);
            }

            break;
        }

        case kRespType_SimpleString:
        case kRespType_Error:
        case kRespType_Integer:
            return find_succ;

        default:
            throw_error(ProtoError, "[Resp] _type = %d", _type);
    }

    return false;
}

bool RespObject::parse_phase2(Buffer *buffer)
{
    size_t sz = buffer->size();
    if (sz == 0)
        return false;

    switch (_type)
    {
        case kRespType_BulkString:
        {
            size_t str_len = std::min(_value_left_length, sz);
            if (str_len > 0)
            {
                buffer->get_string(0, _value, str_len);
                buffer->pop(nullptr, str_len);

                _value_left_length -= str_len;
                sz = buffer->size();
            }

            if (_value_left_length == 0 && sz >= 2)
            {
                buffer->pop(nullptr, 2);
                return true;
            }

            break;
        }

        case kRespType_Array:
        {
            if (_array.empty())
                throw_error(ProtoError, "[Resp] _array.empty");

            while (_array.back().deserialize(buffer))
            {
                --_value_left_length;
                if (_value_left_length == 0)
                    return true;

                _array.push_back(RespObject());
            }

            break;
        }

        default:
            throw_error(ProtoError, "[Resp] _type = %d", _type);
    }

    return false;
}
