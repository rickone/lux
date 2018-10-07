#pragma once

#include <functional>
#include <list>
#include "lua_port.h"

class Entity;

class Component : public LuaObject
{
public:
    Component() = default;
    virtual ~Component() = default;

    static void new_class(lua_State *L);

    virtual void start();
    virtual void stop() noexcept;
    virtual const char * name() const;

    Entity* entity() { return _entity; }
    void set_entity(Entity *entity) { _entity = entity; }

    template<typename T, typename F>
    void set_timer(T *object, F func, int interval, int counter = -1);

protected:
    Entity* _entity;
};

#include "timer.h"

template<typename T, typename F>
void Component::set_timer(T *object, F func, int interval, int counter)
{
    static_assert(std::is_base_of<Component, T>::value, "Component::set_timer failed, T must based on Component");

    auto timer = Timer::create(interval, counter);
    timer->set_callback(object, func);
}
