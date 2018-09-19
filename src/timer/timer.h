#pragma once

#include <memory>
#include "callback.h"

class Timer : public LuaObject
{
public:
    Timer(int interval, int counter);
    Timer(const Timer &other) = default;
    virtual ~Timer() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<Timer> create(int interval, int counter = -1);

    void clear();
    bool trigger();

    int interval() const { return _interval; }
    void set_interval(int interval) { _interval = interval; }

    int counter() const { return _counter; }
    void set_counter(int count) { _counter = count; }

    def_lua_callback(on_timer, Timer *)

private:
    int _interval;
    int _counter;
};
