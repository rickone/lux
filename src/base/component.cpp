#include "component.h"
#include "entity.h"

void Component::new_class(lua_State *L)
{
    lua_new_class(L, Component);

    lua_newtable(L);
    {
        lua_property_readonly(L, entity);
        lua_property_readonly(L, name);

        lua_callback(L, on_error);
    }
    lua_setfield(L, -2, "__property");
}

void Component::lua_init(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    int top = lua_gettop(L);
    int self = top + 1;

    lua_push(L, this);

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_settable(L, self);
    }

    lua_remove(L, -1);
}

void Component::lua_start(lua_State *L)
{
    int top = lua_gettop(L);
    if (!get_luaref(L))
        return;
    
    lua_getfield(L, -1, "start");
    if (!lua_isfunction(L, -1))
    {
        lua_settop(L, top);
        return;
    }

    lua_insert(L, 1);
    lua_replace(L, 2);

    lua_call(L, top, 0);
}

void Component::start()
{
    auto L = get_lua_state();
    if (L)
        lua_start(L);
}

void Component::stop() noexcept
{
    auto L = get_lua_state();
    if (L)
        lua_invoke(L, "stop");
}

const char * Component::name() const
{
    return "noname";
}
