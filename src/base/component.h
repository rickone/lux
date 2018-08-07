#pragma once

#include <functional>
#include <list>
#include "lua_port.h"
#include "timer.h"

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

    template<typename T>
    void set_timer(T *object, const std::function<void (T *, Timer *)> &func, int interval, int counter = -1);

    template<typename T>
    void set_timer(T *object, void (T::*func)(Timer *), int interval, int counter = -1);

protected:
    Entity* _entity;
};

template<typename T>
void Component::set_timer(T *object, const std::function<void (T *, Timer *)> &func, int interval, int counter)
{
    static_assert(std::is_base_of<Component, T>::value, "Component::set_timer failed, T must based on Component");

    auto timer = Timer::create(interval, counter);
    timer->set_callback(object, func);
}

template<typename T>
void Component::set_timer(T *object, void (T::*func)(Timer *), int interval, int counter)
{
    std::function<void (T *, Timer *)> mfn = std::mem_fn(func);
    set_timer(object, mfn, interval, counter);
}

struct LuaMessageObject : public LuaObject
{
    int arg_begin;
    int arg_end;

    virtual int lua_push_self(lua_State *L) override
    {
        for (int i = arg_begin; i <= arg_end; ++i)
        {
            lua_pushvalue(L, i);
        }
        return arg_end - arg_begin + 1;
    }
};

template<typename T>
class Delegate
{
public:
    void add_delegate(T *t)
    {
        _delegate.push_back(t);
    }

protected:
    std::list<T *> _delegate;
};
