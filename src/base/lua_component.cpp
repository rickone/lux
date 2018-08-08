#include "lua_component.h"
#include "timer.h"
//#include "tcp_socket.h"
//#include "udp_socket.h"
//#include "unix_socket.h"

void LuaComponent::new_class(lua_State *L)
{
    lua_new_class(L, LuaComponent);

    lua_newtable(L);
    {
        lua_std_method(L, set_timer);
    }
    lua_setfield(L, -2, "__method");
}

void LuaComponent::init(lua_State *L)
{
    int top = lua_gettop(L);

    luaL_argcheck(L, lua_istable(L, 1), 1, "need a table");

    lua_push(L, this);

    int table = top + 1;

    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_settable(L, table);
    }

    lua_getfield(L, -1, "init");
    if (!lua_isfunction(L, -1))
    {
        lua_settop(L, top);
        return;
    }

    lua_remove(L, 1);
    lua_insert(L, 1);
    lua_insert(L, 2);

    lua_call(L, top, 0);
}

void LuaComponent::start()
{
    invoke("start");
}

void LuaComponent::stop() noexcept
{
    invoke("stop");
}

int LuaComponent::lua_set_timer(lua_State *L)
{
    std::string name(luaL_checkstring(L, 1));
    int interval = (int)luaL_checkinteger(L, 2);
    int counter = (int)luaL_checkinteger(L, 3);

    std::function<void (LuaComponent *, Timer *)> func = [name](LuaComponent *object, Timer *timer){
        object->invoke(name.c_str(), timer);
    };
    set_timer(this, func, interval, counter);
    return 0;
}
