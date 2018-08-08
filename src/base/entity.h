#pragma once

#include <unordered_map>
#include <memory>
#include "component.h"

class Entity final : public LuaObject
{
public:
    Entity() = default;
    ~Entity() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<Entity> create();

    void clear();
    void add_component(const std::shared_ptr<Component> &component);
    std::shared_ptr<Component> get_component(size_t code);
    std::shared_ptr<Component> find_component(const char *name);
    void remove();
    int lua_add_component(lua_State *L);

    bool is_removed() { return _removed; }

    template<typename T, typename...A>
    std::shared_ptr<T> add_component(A...args);

    template<typename T>
    std::shared_ptr<T> get_component();

private:
    std::unordered_map<size_t, std::shared_ptr<Component> > _components;
    bool _removed;
};

template<typename T, typename...A>
std::shared_ptr<T> Entity::add_component(A...args)
{
    auto component = std::make_shared<T>(args...);
    add_component(component);
    return component;
}

template<typename T>
std::shared_ptr<T> Entity::get_component()
{
    auto component = get_component(typeid(T).hash_code());
    if (!component)
        return nullptr;

    return std::static_pointer_cast<T>(component);
}
