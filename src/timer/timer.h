#pragma once

#include <memory>
#include "callback.h"

class Timer : public LuaObject
{
public:
    Timer(uint64_t start_time, unsigned int interval, int counter);
    virtual ~Timer() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<Timer> create(unsigned int interval, int counter = -1, unsigned int delay = 0);

    void clear();
    bool trigger();
    unsigned int duration() const;

    int interval() const { return _interval; }
    void set_interval(int interval) { _interval = interval; }

    int counter() const { return _counter; }
    void set_counter(int count) { _counter = count; }

    def_lua_callback(on_timer)

private:
    uint64_t _start_time;
    unsigned int _interval;
    int _counter;
};
