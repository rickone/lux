#pragma once

#include <functional>
#include "lua_port.h"
#include "callback.h"
#include "log.h"

class Entity;

class Component : public LuaObject
{
public:
    Component() = default;
    virtual ~Component() = default;

    static void new_class(lua_State *L);

    void lua_init(lua_State *L);
    void lua_start(lua_State *L);

    virtual void start();
    virtual void stop() noexcept;
    virtual const char * name() const;

    Entity* entity() { return _entity; }
    void set_entity(Entity *entity) { _entity = entity; }

    template<typename... A>
    void lua_invoke(const char *name, A...args);

protected:
    Entity* _entity;
};

template<typename...A>
void Component::lua_invoke(const char *name, A...args)
{
    lua_State *L = lua_state;
    if (!L)
        return;

    if (!get_luaref(L))
        return;
    
    lua_getfield(L, -1, name);
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        return;
    }

    lua_insert(L, -2);

    int nargs = lua_push_x(L, args...);

    int ret = lua_btcall(L, 1 + nargs, 0);
    if (ret != LUA_OK)
        log_error("[Lua] %s", lua_tostring(L, -1));
}

template<typename... A>
int lua_set_callback(Callback<A...> &cb, lua_State *L)
{
    lua_geti(L, 1, 1);
    Component *object = lua_to<Component *>(L, -1);

    lua_geti(L, 1, 2);
    const char *name = luaL_checkstring(L, -1);

    std::function<void (Component *, A...)> func = [name](Component *object, A...args){
        object->lua_invoke(name, args...);
    };
    cb.set(object, func);
    return 0;
}

#define def_lua_callback(callback, ...) \
    Callback<__VA_ARGS__> callback; \
    int lua_set_##callback(lua_State *L) { return lua_set_callback(callback, L); }

#define lua_callback(L, callback) \
    lua_newtable(L); \
    { \
        lua_set_method(L, "set", lua_set_##callback); \
    } \
    lua_setfield(L, -2, #callback)
