#pragma once

#include <string>

template<typename T>
struct LuaBridge;

template<typename T>
inline T lua_to(lua_State *L, int index)
{
    return LuaBridge<T>::to(L, index);
}

template<typename T>
inline int lua_push(lua_State *L, T value)
{
    return LuaBridge<T>::push(L, value);
}

#define lua_bridge_def(c_type, lua_type, to_func, push_func) \
    template<> \
    struct LuaBridge<c_type> \
    { \
        static c_type to(lua_State *L, int index) \
        { \
            luaL_checktype(L, index, lua_type); \
            return (c_type)(to_func(L, index)); \
        } \
        static int push(lua_State *L, c_type value) \
        { \
            push_func(L, value); \
            return 1; \
        } \
    }

lua_bridge_def(int, LUA_TNUMBER, lua_tointeger, lua_pushinteger);
lua_bridge_def(unsigned int, LUA_TNUMBER, lua_tointeger, lua_pushinteger);

lua_bridge_def(short, LUA_TNUMBER, lua_tointeger, lua_pushinteger);
lua_bridge_def(unsigned short, LUA_TNUMBER, lua_tointeger, lua_pushinteger);

lua_bridge_def(long, LUA_TNUMBER, lua_tointeger, lua_pushinteger);
lua_bridge_def(unsigned long, LUA_TNUMBER, lua_tointeger, lua_pushinteger);

lua_bridge_def(long long, LUA_TNUMBER, lua_tointeger, lua_pushinteger);
lua_bridge_def(unsigned long long, LUA_TNUMBER, lua_tointeger, lua_pushinteger);

lua_bridge_def(char, LUA_TNUMBER, lua_tointeger, lua_pushinteger);
lua_bridge_def(unsigned char, LUA_TNUMBER, lua_tointeger, lua_pushinteger);

lua_bridge_def(float, LUA_TNUMBER, lua_tonumber, lua_pushnumber);
lua_bridge_def(double, LUA_TNUMBER, lua_tonumber, lua_pushnumber);

lua_bridge_def(bool, LUA_TBOOLEAN, 0 != lua_toboolean, lua_pushboolean);

lua_bridge_def(const char*, LUA_TSTRING, lua_tostring, lua_pushstring);

template<>
struct LuaBridge<const std::string &>
{
    static std::string to(lua_State *L, int index)
    {
        size_t len = 0;
        const char *data = luaL_checklstring(L, index, &len);
        return std::string(data, len);
    }
    static int push(lua_State *L, const std::string &str)
    {
        lua_pushlstring(L, str.data(), str.size());
        return 1;
    }
};

template<>
struct LuaBridge<std::string>
{
    static std::string to(lua_State *L, int index)
    {
        size_t len = 0;
        const char *data = luaL_checklstring(L, index, &len);
        return std::string(data, len);
    }
    static int push(lua_State *L, const std::string &str)
    {
        lua_pushlstring(L, str.data(), str.size());
        return 1;
    }
};
