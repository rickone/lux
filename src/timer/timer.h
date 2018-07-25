#pragma once

#include <memory>
#include "lua_port.h"

class Timer : public LuaObject
{
public:
    Timer(int interval, int counter);
    Timer(const Timer &other) = default;
    virtual ~Timer() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<Timer> create(int interval, int counter = -1);

    bool trigger();
    void stop();

    int interval() const { return _interval; }
    void set_interval(int interval) { _interval = interval; }

    int counter() const { return _counter; }
    void set_counter(int count) { _counter = count; }

    template<typename T>
    void set_callback(T *object, const std::function<void (T *, Timer *)> &func);

    template<typename T>
    void set_callback(T *object, void (T::*func)(Timer *));

private:
    int _interval;
    int _counter;
    std::weak_ptr<LuaObject> _callback_object;
    std::function<void (LuaObject *, Timer *)> _callback_func;
};

template<typename T>
void Timer::set_callback(T *object, const std::function<void (T *, Timer *)> &func)
{
    static_assert(std::is_base_of<LuaObject, T>::value, "Timer::set_callback failed, T must based on LuaObject");

    _callback_object = object->shared_from_this();
    _callback_func = [func](LuaObject *object, Timer *timer){
        func((T *)object, timer);
    };
}

template<typename T>
void Timer::set_callback(T *object, void (T::*func)(Timer *))
{
    std::function<void (T *, Timer *)> mfn = std::mem_fn(func);
    set_callback(object, mfn);
}
