#pragma once

#include "component.h"
#include "entity.h"
#include "lua_port.h"
#include "log.h"
#include "callback.h"

class LuaComponent : public Component
{
public:
    LuaComponent() = default;
    virtual ~LuaComponent() = default;
    
    static void new_class(lua_State *L);
    
    void init(lua_State *L);

    template<typename... A>
    void invoke(const char *name, A...args);

    virtual void start() override;
    virtual void stop() noexcept override;

    int lua_set_timer(lua_State *L);
};

template<typename...A>
void LuaComponent::invoke(const char *name, A...args)
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
    LuaComponent *object = lua_to<LuaComponent *>(L, -1);

    lua_geti(L, 1, 2);
    const char *name = luaL_checkstring(L, -1);

    std::function<void (LuaComponent *, A...)> func = [name](LuaComponent *object, A...args){
        object->invoke(name, args...);
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
