#include "entity.h"
#include <cstring> // strcmp
#include "world.h"

void Entity::new_class(lua_State *L)
{
    lua_new_class(L, Entity);

    lua_newtable(L);
    {
        lua_method(L, remove);
        lua_std_method(L, add_component);
        lua_std_method(L, get_component);
        lua_std_method(L, find_component);
        lua_method(L, add_timer);
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
    return World::inst()->create_entity();
}

void Entity::clear()
{
    _components.clear();
}

void Entity::add_component(const std::shared_ptr<Component> &component)
{
    auto ptr = component.get();
    size_t code = typeid(*ptr).hash_code();
    _components.push_back(std::make_pair(code, component));
    component->set_entity(this);
    component->start();
}

std::shared_ptr<Component> Entity::get_component(size_t code)
{
    auto it = _components.begin();
    auto it_end = _components.end();
    for (; it != it_end; ++it)
    {
        if (it->first == code)
            return it->second;
    }
    return nullptr;
}

int Entity::get_components(size_t code, const std::function<void(std::shared_ptr<Component> &)> &func)
{
    int count = 0;
    auto it = _components.begin();
    auto it_end = _components.end();
    for (; it != it_end; ++it)
    {
        if (it->first == code)
        {
            ++count;
            func(it->second);
        }
    }
    return count;
}

std::shared_ptr<Component> Entity::find_component(const char *name)
{
    auto it = _components.begin();
    auto it_end = _components.end();
    for (; it != it_end; ++it)
    {
        if (strcmp(it->second->name(), name) == 0)
            return it->second;
    }

    return nullptr;
}

void Entity::remove()
{
    if (_removed)
        return;

    for (auto &pair : _components)
        pair.second->stop();

    _removed = true;
}

int Entity::lua_add_component(lua_State *L)
{
    std::shared_ptr<Component> component = lua_to_shared_ptr_safe<Component>(L, 1);
    if (component)
    {
        add_component(component);
        lua_pushvalue(L, 1);
    }
    else if (lua_istable(L, 1))
    {
        component.reset(new Component());
        component->lua_init(L);

        add_component(component);
        lua_push(L, component);
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

std::shared_ptr<Timer> Entity::add_timer(int interval, int counter)
{
    auto timer = Timer::create(interval, counter);
    //add_component(timer);
    return timer;
}
