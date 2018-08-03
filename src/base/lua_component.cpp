#include "lua_component.h"
#include "timer.h"
#include "buffer.h"
#include "tcp_socket.h"
#include "udp_socket.h"
#include "unix_socket.h"

LuaComponent::LuaComponent()
{
}

LuaComponent::~LuaComponent()
{
}

void LuaComponent::new_class(lua_State *L)
{
    lua_new_class(L, LuaComponent);

    lua_newtable(L);
    {
        lua_std_method(L, subscribe);
        lua_std_method(L, publish);
        lua_std_method(L, set_timer);
    }
    lua_setfield(L, -2, "__method");
}

int LuaComponent::attach(lua_State *L, int index)
{
    if (index < 0)
        index += lua_gettop(L) + 1;

    luaL_argcheck(L, lua_istable(L, index), index, "need a table");

    lua_push(L, this);

    int table = lua_gettop(L);

    lua_pushnil(L);
    while (lua_next(L, index))
    {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_settable(L, table);
    }

    return 0;
}

void LuaComponent::on_lua_custom(void *state)
{
    lua_State *L = (lua_State *)state;
    if (!L)
        return;

    const char *name = luaL_checkstring(L, 1);
    int top = lua_gettop(L);

    if (!get_luaref(L))
        return;

    lua_getfield(L, -1, name);
    if (!lua_isfunction(L, -1))
    {
        lua_settop(L, top);
        return;
    }

    lua_remove(L, -2);
    lua_push(L, this);
    for (int i = 2; i <= top; ++i)
        lua_pushvalue(L, i);

    int ret = lua_btcall(L, top, 0);
    if (ret != LUA_OK)
    {
        log_error("[Lua] %s", lua_tostring(L, -1));
        lua_settop(L, top);
    }
}

void LuaComponent::start(LuaObject *init_object)
{
    invoke("start", init_object);
}

void LuaComponent::stop() noexcept
{
    invoke("stop");
}

int LuaComponent::lua_subscribe(lua_State *L)
{
    int msg_type = luaL_checkinteger(L, 1);
    std::string msg_func(luaL_checkstring(L, 2));

    subscribe(msg_type, [this, msg_func](LuaObject *msg_object){
        invoke(msg_func.c_str(), msg_object);
    });
    return 0;
}

int LuaComponent::lua_publish(lua_State *L)
{
    int msg_type = luaL_checkinteger(L, 1);

    LuaMessageObject msg_object;
    msg_object.arg_begin = 2;
    msg_object.arg_end = lua_gettop(L);

    publish(msg_type, &msg_object);
    return 0;
}

int LuaComponent::lua_set_timer(lua_State *L)
{
    std::string name(luaL_checkstring(L, 1));
    int interval = luaL_checkinteger(L, 2);
    int counter = luaL_checkinteger(L, 3);

    std::function<void (LuaComponent *, Timer *)> func = [name](LuaComponent *object, Timer *timer){
        object->invoke(name.c_str(), timer);
    };
    set_timer(this, func,interval, counter);
    return 0;
}
