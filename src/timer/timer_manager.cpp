#include "timer_manager.h"
#include <cassert>

TimerManager *timer_manager = nullptr;

inline int64_t lux_gettime()
{
    struct timespec ts;
    int ret = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    if (ret == -1)
        throw_system_error(errno, "clock_gettime");

    return (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1'000'000;
}

TimerManager::TimerManager() : _skip_list(2), _time_now()
{
    assert(timer_manager == nullptr);
    timer_manager = this;
}

std::shared_ptr<Timer> TimerManager::create(int interval, int counter)
{
    logic_assert(interval > 0, "interval = %d", interval);

    std::shared_ptr<Timer> timer(new Timer(interval, counter));
    int64_t key = _time_now + interval;
    _skip_list.create(key, timer);

    return timer;
}

int64_t TimerManager::time_now()
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

        int64_t key = node->get_key();
        int64_t interval = (int64_t)timer->interval();
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
