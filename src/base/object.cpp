#include "object.h"
#include "lua_port.h"

using namespace lux;

Object::~Object()
{
    auto L = get_lua_state();;
    if (!L)
        return;

    if (!get_luaref(L))
        return;
    
    if (lua_istable(L, -1))
    {
        lua_pushnil(L);
        lua_setfield(L, -2, "__ptr");
    }
    lua_pop(L, 1);

    release_luaref(L);
}

bool Object::get_luaref(lua_State *L)
{
    if (_ref == LUA_NOREF)
        return false;
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, _ref);
    return true;
}

void Object::retain_luaref(lua_State *L)
{
    lua_pushvalue(L, -1);

    release_luaref(L);
    _ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

void Object::release_luaref(lua_State *L)
{
    if (_ref == LUA_NOREF)
        return;

    luaL_unref(L, LUA_REGISTRYINDEX, _ref);
    _ref = LUA_NOREF;
}

int Object::lua_push_self(lua_State *L)
{
    return 0;
}

bool Object::is_valid()
{
    return true;
}
