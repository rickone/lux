#include "timer.h"
#include "timer_manager.h"
#include "log.h"

Timer::Timer(uint64_t start_time, unsigned int interval, int counter) : _start_time(start_time), _interval(interval), _counter(counter)
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
        lua_property_readonly(L, duration);
        lua_property(L, interval);
        lua_property(L, counter);

        lua_callback(L, on_timer);
    }
    lua_setfield(L, -2, "__property");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_timer", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<Timer> Timer::create(unsigned int interval, int counter, unsigned int delay)
{
    return TimerManager::inst()->create(interval, counter, delay);
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
        on_timer();
    }
    catch (const std::runtime_error &err)
    {
        log_error("%s", err.what());
    }
    return _counter != 0;
}

unsigned int Timer::duration() const
{
    return (unsigned int)(TimerManager::inst()->time_now() - _start_time);
}
