#pragma once

#include <functional>
#include "callback.h"

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

    def_lua_callback(on_error, Component *, int)

protected:
    Entity* _entity;
};

template<typename...A>
void Component::lua_invoke(const char *name, A...args)
{
    lua_State *L = lua_state;
    if (!L)
        return;

    int top = lua_gettop(L);
    if (!get_luaref(L))
        return;
    
    lua_getfield(L, -1, name);
    if (!lua_isfunction(L, -1))
    {
        lua_settop(L, top);
        return;
    }

    lua_insert(L, -2);

    int nargs = lua_push_x(L, args...);

    lua_call(L, 1 + nargs, 0);
}
