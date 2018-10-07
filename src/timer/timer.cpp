#include "timer.h"
#include "timer_manager.h"
#include "component.h"

Timer::Timer(int interval, int counter) : _interval(interval), _counter(counter), _callback()
{
}

void Timer::new_class(lua_State *L)
{
    lua_new_class(L, Timer);

    lua_newtable(L);
    {
        lua_method(L, stop);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_property(L, interval);
        lua_property(L, counter);
    }
    lua_setfield(L, -2, "__property");
}

std::shared_ptr<Timer> Timer::create(int interval, int counter)
{
    return timer_manager->create(interval, counter);
}

bool Timer::trigger()
{
    if (_counter == 0)
        return false;

    if (_counter > 0)
        _counter--;

    try
    {
        _callback(this);
    }
    catch (const std::runtime_error &err)
    {
        log_error("%s", err.what());
    }
    return _counter != 0;
}

void Timer::stop()
{
    _interval = 0;
    _counter = 0;
    _callback.clear();
}
