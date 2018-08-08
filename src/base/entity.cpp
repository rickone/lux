#include "entity.h"
#include <cstring> // strcmp
#include "world.h"
#include "lua_component.h"

void Entity::new_class(lua_State *L)
{
    lua_new_class(L, Entity);

    lua_newtable(L);
    {
        lua_method(L, remove);
        lua_std_method(L, add_component);
        lua_method_sn(L, get_component, std::shared_ptr<Component> (Entity::*)(size_t code));
    }
    lua_setfield(L, -2, "__method");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_entity", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<Entity> Entity::create()
{
    return world->create_object();
}

void Entity::clear()
{
    for (auto it = _components.begin(); it != _components.end(); ++it)
    {
        it->second->stop();
    }
    _components.clear();
}

void Entity::add_component(const std::shared_ptr<Component> &component)
{
    size_t code = typeid(*component.get()).hash_code();
    _components[code] = component;
    component->set_entity(this);
    component->start();
}

std::shared_ptr<Component> Entity::get_component(size_t code)
{
    auto it = _components.find(code);
    if (it == _components.end())
        return nullptr;

    return it->second;
}

std::shared_ptr<Component> Entity::find_component(const char *name)
{
    for (auto it = _components.begin(); it != _components.end(); ++it)
    {
        if (strcmp(it->second->name(), name) == 0)
            return it->second;
    }

    return nullptr;
}

void Entity::remove()
{
    _removed = true;
}

int Entity::lua_add_component(lua_State *L)
{
    auto component = lua_to_shared_ptr_safe<Component>(L, 1);
    if (component)
    {
        lua_pushvalue(L, 1);

        add_component(component);
    }
    else if (lua_istable(L, 1))
    {
        std::shared_ptr<LuaComponent> lua_component(new LuaComponent());
        lua_component->init(L);

        add_component(lua_component);
    }
    else
    {
        return luaL_argerror(L, 1, "need a table or an object");
    }

    return 1;
}
