#include "timer.h"
#include "timer_manager.h"
#include "component.h"

Timer::Timer(int interval, int counter) : _interval(interval), _counter(counter)
{
}

void Timer::new_class(lua_State *L)
{
    lua_new_class(L, Timer);

    lua_newtable(L);
    {
        lua_method(L, clear);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_property(L, interval);
        lua_property(L, counter);

        lua_callback(L, on_timer);
    }
    lua_setfield(L, -2, "__property");
}

std::shared_ptr<Timer> Timer::create(int interval, int counter)
{
    return TimerManager::inst()->create(interval, counter);
}

void Timer::clear()
{
    _interval = 0;
    _counter = 0;
    on_timer.clear();
}

bool Timer::trigger()
{
    if (_counter == 0)
        return false;

    if (_counter > 0)
        _counter--;

    try
    {
        on_timer(this);
    }
    catch (const std::runtime_error &err)
    {
        log_error("%s", err.what());
    }
    return _counter != 0;
}
