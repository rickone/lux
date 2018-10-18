#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "lua_object.h"
#include "timer.h"

class ObjectManager final
{
public:
    ObjectManager() = default;
    ~ObjectManager() = default;

    static ObjectManager * inst();

    void add_object(const std::shared_ptr<LuaObject> &object);
    std::shared_ptr<LuaObject> get_object(int id);
    void gc();

    template<typename T, typename...A>
    std::shared_ptr<T> create(A...args);

private:
    std::unordered_map<int, std::shared_ptr<LuaObject> > _objects;
    int _last_id = 0;
};

template<typename T, typename... A>
std::shared_ptr<T> ObjectManager::create(A... args)
{
    static_assert(std::is_base_of<LuaObject, T>::value, "ObjectManager::create failed, need a LuaObject-based type");

    auto object = std::make_shared<T>(args...);
    add_object(object);
    return object;
}
