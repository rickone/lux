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
        lua_std_method(L, get_component);
        lua_std_method(L, find_component);
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
    auto ptr = component.get();
    size_t code = typeid(*ptr).hash_code();
    _components.emplace(code, component);
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

int Entity::get_components(size_t code, const std::function<void(std::shared_ptr<Component> &)> &func)
{
    int count = 0;
    auto range = _components.equal_range(code);
    for (auto it = range.first; it != range.second; ++it)
    {
        ++count;
        func(it->second);
    }
    return count;
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
        lua_push(L, lua_component);
    }
    else
    {
        return luaL_argerror(L, 1, "need a table or an object");
    }

    return 1;
}

int Entity::lua_get_component(lua_State *L)
{
    const char *class_name = luaL_checkstring(L, 1);

    lua_getglobal(L, "class_info");
    lua_getfield(L, -1, class_name);
    if (!lua_istable(L, -1))
        return 0;

    lua_getfield(L, -1, "code");
    size_t code = (size_t)luaL_checkinteger(L, -1);

    return get_components(code, [L](std::shared_ptr<Component> &component){
        lua_push(L, component.get());
    });
}

int Entity::lua_find_component(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    auto component = find_component(name);
    if (!component)
        return 0;

    lua_push(L, component.get());
    return 1;
}
