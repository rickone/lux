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
    void publish_msg(int msg_type, LuaObject *msg_object);
    void remove();

    int lua_add_component(lua_State *L);

    bool is_removed() { return _removed; }

    template<typename T, typename...A>
    void publish(int msg_type, T t, A...args);

    template<typename T>
    void publish(int msg_type, T t);

private:
    std::list< std::shared_ptr<Component> > _components;
    bool _removed;
};

template<typename T, typename...A>
void Entity::publish(int msg_type, T t, A...args)
{
    LuaObjectList object_list;

    lua_obj_list_push(object_list.list, t, args...);
    publish_msg(msg_type, &object_list);
}

template<typename T>
void Entity::publish(int msg_type, T t)
{
    static_assert(std::is_pointer<T>::value, "Entity::publish failed, need a pointer");
    static_assert(std::is_base_of<LuaObject, typename std::remove_pointer<T>::type>::value, "Entity::publish failed, need a LuaObject-based object");

    publish_msg(msg_type, t);
}
