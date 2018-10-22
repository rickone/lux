#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "object.h"
#include "timer.h"

namespace lux {

class ObjectManager final
{
public:
    ObjectManager() = default;
    ~ObjectManager() = default;

    static ObjectManager *inst();

    void add_object(const std::shared_ptr<Object> &object);
    std::shared_ptr<Object> get_object(int id);
    void gc();

    template<typename T, typename...A>
    std::shared_ptr<T> create(A...args);

private:
    std::unordered_map<int, std::shared_ptr<Object> > _objects;
    int _last_id = 0;
};

template<typename T, typename... A>
std::shared_ptr<T> ObjectManager::create(A... args)
{
    static_assert(std::is_base_of<Object, T>::value, "ObjectManager::create failed, need a Object-based type");

    auto object = std::make_shared<T>(args...);
    add_object(object);
    return object;
}

} // lux
