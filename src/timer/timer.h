#pragma once

#include <memory>
#include "lua_port.h"
#include "callback.h"

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

    template<typename T, typename F>
    void set_callback(T *object, F func) { _callback.set(object, func); }

private:
    int _interval;
    int _counter;
    Callback<Timer *> _callback;
};
