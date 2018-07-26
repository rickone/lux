#include "luap_object.h"
#include <cassert>
#include <algorithm>
#include "lua_port.h"
#include "buffer.h"
#include "variant_int.h"

/* Header Byte Code
 *
 * Varient Int:
 * 0xxx,xxxx -- 1B
 * 10xx,xxxx -> [1xxx,xxxx] -> [0xxx,xxxx] -- nB littile-endian
 * 1100,0000 -- 1B type
 *
 * 0xC0 - nil
 * 0xC1 - false
 * 0xC2 - true
 * 0xC3 - number
 * 0xC4 - string
 * 0xC5 - table
 * 0xC6 - args
 */

void LuapObject::new_class(lua_State *L)
{
    lua_new_class(L, LuapObject);

    lua_newtable(L);
    {
        lua_method(L, clear);
        lua_method(L, pack_args);
    }
    lua_setfield(L, -2, "__method");

    /*
    lua_newtable(L);
    {
        lua_method(L, create);
    }
    lua_setglobal(L, "luap");
    */
}

std::shared_ptr<LuapObject> LuapObject::create()
{
    return std::shared_ptr<LuapObject>(new LuapObject());
}

void LuapObject::clear()
{
    _type = kLuapType_Nil;
    _str_value.clear();
    _table.clear();
    _phase = 0;
    _value_left_length = 0;
}

void LuapObject::set_nil()
{
    _type = kLuapType_Nil;
}

void LuapObject::set_boolean(bool value)
{
    _type = kLuapType_Boolean;
    _value.boolean = value;
}

void LuapObject::set_integer(long value)
{
    _type = kLuapType_Integer;
    _value.integer = value;
}

void LuapObject::set_number(double value)
{
    _type = kLuapType_Number;
    _value.number = value;
}

void LuapObject::set_string(const char *data, size_t len)
{
    _type = kLuapType_String;
    _str_value.assign(data, len);
}

void LuapObject::set_table()
{
    _type = kLuapType_Table;
    _table.clear();
}

void LuapObject::table_set(const LuapObject &key, const LuapObject &value)
{
    if (_type != kLuapType_Table)
        throw_error(ProtoError, "[Luap] _type = %d", _type);

    _table.push_back(key);
    _table.push_back(value);
}

void LuapObject::set_args()
{
    _type = kLuapType_Args;
    _table.clear();
}

void LuapObject::args_push(const LuapObject &obj)
{
    if (_type != kLuapType_Args)
        throw_error(ProtoError, "[Luap] _type = %d", _type);

    _table.push_back(obj);
}

void LuapObject::serialize(Buffer *buffer)
{
    switch (_type)
    {
        case kLuapType_Nil:
        {
            buffer->push("\xC0", 1);
            break;
        }

        case kLuapType_Boolean:
        {
            if (_value.boolean)
                buffer->push("\xC2", 1);
            else
                buffer->push("\xC1", 1);
            break;
        }

        case kLuapType_Integer:
        {
            variant_int_write(_value.integer, buffer);
            break;
        }

        case kLuapType_Number:
        {
            buffer->push("\xC3", 1);
            buffer->push((const char *)&_value.number, sizeof(_value.number));
            break;
        }

        case kLuapType_String:
        {
            buffer->push("\xC4", 1);
            variant_int_write((long)_str_value.size(), buffer);
            buffer->push(_str_value.data(), _str_value.size());
            break;
        }

        case kLuapType_Table:
        {
            buffer->push("\xC5", 1);
            variant_int_write((long)_table.size(), buffer);

            for (LuapObject &obj : _table)
                obj.serialize(buffer);
            break;
        }

        case kLuapType_Args:
        {
            buffer->push("\xC6", 1);
            variant_int_write((long)_table.size(), buffer);

            for (LuapObject &obj : _table)
                obj.serialize(buffer);
            break;
        }

        default:
            assert(false);
    }
}

bool LuapObject::deserialize(Buffer *buffer)
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
            throw_error(ProtoError, "[Luap] _phase = %d", _phase);
    }
}

int LuapObject::pack(lua_State *L)
{
    int index = lua_gettop(L);
    int type = lua_type(L, index);

    switch (type)
    {
        case LUA_TNIL:
        {
            set_nil();
            break;
        }

        case LUA_TNUMBER:
        {
            if (lua_isinteger(L, index))
            {
                long value = (long)lua_tointeger(L, index);
                set_integer(value);
            }
            else
            {
                double value = lua_tonumber(L, index);
                set_number(value);
            }
            break;
        }

        case LUA_TBOOLEAN:
        {
            bool value = (bool)lua_toboolean(L, index);
            set_boolean(value);
            break;
        }

        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *data = lua_tolstring(L, index, &len);
            set_string(data, len);
            break;
        }

        case LUA_TTABLE:
        {
            set_table();

            lua_pushnil(L);
            while (lua_next(L, index))
            {
                lua_pushvalue(L, -2);

                LuapObject key;
                key.pack(L);

                LuapObject value;
                value.pack(L);

                table_set(key, value);
            }

            break;
        }

        default:
            luaL_error(L, "LuapObject::pack unknown type: %d", type);
    }

    lua_pop(L, 1);
    return 1;
}

int LuapObject::unpack(lua_State *L)
{
    switch (_type)
    {
        case kLuapType_Nil:
        {
            lua_pushnil(L);
            break;
        }

        case kLuapType_Boolean:
        {
            lua_pushboolean(L, _value.boolean);
            break;
        }

        case kLuapType_Integer:
        {
            lua_pushinteger(L, _value.integer);
            break;
        }

        case kLuapType_Number:
        {
            lua_pushnumber(L, _value.number);
            break;
        }

        case kLuapType_String:
        {
            lua_pushlstring(L, _str_value.data(), _str_value.size());
            break;
        }

        case kLuapType_Table:
        {
            int count = (int)_table.size();

            lua_newtable(L);
            for (int i = 0; i + 1 < count; i += 2)
            {
                _table[i].unpack(L);
                _table[i + 1].unpack(L);
                lua_rawset(L, -3);
            }
            break;
        }

        case kLuapType_Args:
        {
            int count = (int)_table.size();

            for (int i = 0; i < count; ++i)
                _table[i].unpack(L);

            return count;
        }

        default:
            assert(false);
    }

    clear();
    return 1;
}

int LuapObject::pack_args(lua_State *L)
{
    set_args();

    int top = lua_gettop(L);
    for (int i = 1; i <= top; ++i)
    {
        lua_pushvalue(L, i);

        LuapObject value;
        value.pack(L);

        args_push(value);
    }

    return 0;
}

bool LuapObject::parse_phase0(Buffer *buffer)
{
    size_t sz = buffer->size();
    if (sz == 0)
        return false;

    uint8_t header = *(uint8_t *)buffer->data(0);
    uint8_t tag = (header & 0xC0);

    if (tag != 0xC0)
    {
        long value = 0;
        size_t var_len = variant_int_read(buffer, &value);
        if (var_len == 0)
            return false;

        set_integer(value);
        buffer->pop(nullptr, var_len);
        return true;
    }

    buffer->pop(nullptr, 1);
    switch (header)
    {
        case 0xC0:
        {
            set_nil();
            return true;
        }

        case 0xC1:
        {
            set_boolean(false);
            return true;
        }

        case 0xC2:
        {
            set_boolean(true);
            return true;
        }

        case 0xC3:
        {
            set_number(0);
            break;
        }

        case 0xC4:
        {
            set_string("", 0);
            break;
        }

        case 0xC5:
        {
            set_table();
            break;
        }

        case 0xC6:
        {
            set_args();
            break;
        }

        default:
            throw_error(ProtoError, "[Luap] header = 0x%02X", header);
    }

    ++_phase;
    return parse_phase1(buffer);
}

bool LuapObject::parse_phase1(Buffer *buffer)
{
    size_t sz = buffer->size();
    if (sz == 0)
        return false;

    switch (_type)
    {
        case kLuapType_Number:
        {
            if (sz < sizeof(double))
                return false;

            double number = 0;
            buffer->get(0, (char *)&number, sizeof(number));

            set_number(number);
            buffer->pop(nullptr, sizeof(number));
            return true;
        }

        case kLuapType_String:
        {
            long value = 0;
            size_t var_len = variant_int_read(buffer, &value);
            if (var_len == 0)
                return false;

            buffer->pop(nullptr, var_len);

            if (value < 0)
                throw_error(ProtoError, "[Luap] value < 0");

            if (value == 0)
                return true;

            _value_left_length = (size_t)value;

            ++_phase;
            return parse_phase2(buffer);
        }

        case kLuapType_Table:
        case kLuapType_Args:
        {
            long value = 0;
            size_t var_len = variant_int_read(buffer, &value);
            if (var_len == 0)
                return false;

            buffer->pop(nullptr, var_len);

            if (value < 0)
                throw_error(ProtoError, "[Luap] value < 0");

            if (value == 0)
                return true;

            _value_left_length = (size_t)value;
            _table.push_back(LuapObject());

            ++_phase;
            return parse_phase2(buffer);
        }

        default:
            throw_error(ProtoError, "[Luap] _type = %d", _type);
    }

    return false;
}

bool LuapObject::parse_phase2(Buffer *buffer)
{
    size_t sz = buffer->size();
    if (sz == 0)
        return false;

    switch (_type)
    {
        case kLuapType_String:
        {
            size_t str_len = std::min(_value_left_length, sz);
            buffer->get_string(0, _str_value, str_len);
            buffer->pop(nullptr, str_len);

            _value_left_length -= str_len;
            if (_value_left_length == 0)
                return true;

            break;
        }

        case kLuapType_Table:
        case kLuapType_Args:
        {
            if (_table.back().deserialize(buffer))
            {
                _value_left_length--;
                if (_value_left_length == 0)
                    return true;

                _table.push_back(LuapObject());
            }
            break;
        }

        default:
            throw_error(ProtoError, "[Luap] _type = %d", _type);
    }

    return false;
}
