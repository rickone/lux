#include "timer_manager.h"
#include <cassert>

#ifdef _WIN32
#include <windows.h>
#define lux_gettime GetTickCount64
#elif __APPLE__
#include <sys/time.h>
static uint64_t lux_gettime()
{
    struct timeval tv;
    int ret = gettimeofday(&tv, nullptr);
    if (ret == -1)
        throw_unix_error("gettimeofday");

    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}
#else
#include <sys/time.h>
static uint64_t lux_gettime()
{
    struct timespec ts;
#ifdef __linux__
    int ret = clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
#else
    int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    if (ret == -1)
        throw_unix_error("clock_gettime");

    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1'000'000;
}
#endif

TimerManager::TimerManager() : _skip_list(2)
{
}

TimerManager * TimerManager::inst()
{
    static TimerManager s_inst;
    return &s_inst;
}

void TimerManager::init()
{
    _time_now = lux_gettime();
}

std::shared_ptr<Timer> TimerManager::create(unsigned int interval, int counter, unsigned int delay)
{
    logic_assert(interval > 0, "interval = %d", interval);

    auto timer = std::make_shared<Timer>(_time_now, interval, counter);
    uint64_t key = _time_now + delay;
    _skip_list.create(key, timer);

    return timer;
}

uint64_t TimerManager::time_now()
{
    return _time_now;
}

int TimerManager::tick()
{
    _time_now = lux_gettime();
    if (_time_now < _next_tick_time)
        return (int)(_next_tick_time - _time_now);

    unsigned int n = _skip_list.upper_rank(_time_now);
    auto node = _skip_list.remove(0, n);
    while (node)
    {
        auto next = node->forward(0);
        auto &timer = node->get_value();

        uint64_t key = node->get_key();
        uint64_t interval = (uint64_t)timer->interval();
        bool timeup = false;
        do
        {
            if (!timer->trigger()) // maybe multi-times
            {
                timeup = true;
                break;
            }

            key += interval;
        } while (key <= _time_now);

        if (timeup)
        {
            node->purge();
        }
        else
        {
            node->set_key(key);
            _skip_list.insert(node);
        }

        node = next;
    }

    auto first_node = _skip_list.first_node();
    if (!first_node)
    {
        _next_tick_time = _time_now + 100;
        return 100;
    }

    _next_tick_time = first_node->get_key();
    return (int)(_next_tick_time - _time_now);
}
