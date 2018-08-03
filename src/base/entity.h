#pragma once

#include <list>
#include <memory>
#include "component.h"

class Entity final : public LuaObject
{
public:
    Entity() = default;
    ~Entity() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<Entity> create();

    void gc();
    void add_component(const std::shared_ptr<Component> &component, LuaObject *init_object = nullptr);
    std::shared_ptr<Component> find_component(const char *name);
    void publish(int msg_type, LuaObject *msg_object);
    void remove();

    int lua_add_component(lua_State *L);

    bool is_removed() { return _removed; }

private:
    std::list< std::shared_ptr<Component> > _components;
    bool _removed;
};
