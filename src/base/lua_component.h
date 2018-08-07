#pragma once

#include "component.h"
#include "entity.h"
#include "lua_port.h"
#include "log.h"

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

#define invoke_delegate(method, ...) \
    do \
    { \
        for (auto dlgt : _delegate) \
            dlgt->method(__VA_ARGS__); \
        auto component = _entity->get_component<LuaComponent>(); \
        if (component) \
            component->invoke(#method,## __VA_ARGS__); \
    } while (false)
