#pragma once

#include "component.h"
#include "lua_port.h"
#include "log.h"

class LuaComponent : public Component
{
public:
    LuaComponent();
    virtual ~LuaComponent();
    
    static void new_class(lua_State *L);
    
    int attach(lua_State *L, int index);
    void subscribe_custom();
    void on_lua_custom(void *state);

    template<typename... A>
    void invoke(const char *name, A...args);

    virtual void start(LuaObject *init_object) override;
    virtual void stop() noexcept override;

    int lua_subscribe(lua_State *L);
    int lua_publish(lua_State *L);
    int lua_set_timer(lua_State *L);

private:
    int _table_ref;
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

    lua_remove(L, -2);

    int nargs = lua_push(L, this);
    nargs += lua_push_x(L, args...);

    int ret = lua_btcall(L, nargs, 0);
    if (ret != LUA_OK)
        log_error("[Lua] %s", lua_tostring(L, -1));
}
