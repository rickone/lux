#include "entity.h"
#include "world.h"
#include "lua_component.h"

void Entity::new_class(lua_State *L)
{
    lua_new_class(L, Entity);

    lua_newtable(L);
    {
        lua_method(L, find_component);
        lua_method(L, remove);
        lua_std_method(L, add_component);
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

void Entity::gc()
{
    for (auto it = _components.begin(); it != _components.end(); )
    {
        auto &component = *it;
        if (component->is_removed())
        {
            _components.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

void Entity::add_component(const std::shared_ptr<Component> &component, LuaObject *init_object)
{
    _components.push_back(component);
    component->set_entity(this);
    component->start(init_object);
}

std::shared_ptr<Component> Entity::find_component(const char *name)
{
    for (auto &component : _components)
    {
        if (component->name() == name)
            return component;
    }

    return nullptr;
}

void Entity::publish(int msg_type, LuaObject *msg_object)
{
    for (auto &component : _components)
    {
        component->on_msg(msg_type, msg_object);
    }
}

void Entity::remove()
{
    for (auto &component : _components)
    {
        component->remove();
    }
    _removed = true;
}

int Entity::lua_add_component(lua_State *L)
{
    auto component = lua_to_shared_ptr_safe<Component>(L, 1);
    if (component)
    {
        lua_pushvalue(L, 1);

        LuaMessageObject object;
        object.arg_begin = 2;
        object.arg_end = lua_gettop(L);

        add_component(component, &object);
    }
    else if (lua_istable(L, 1))
    {
        std::shared_ptr<LuaComponent> lua_component(new LuaComponent());
        lua_component->attach(L, 1);

        LuaMessageObject object;
        object.arg_begin = 2;
        object.arg_end = lua_gettop(L);

        add_component(lua_component, &object);
    }
    else
    {
        return luaL_argerror(L, 1, "need a table or an object");
    }

    return 1;
}
