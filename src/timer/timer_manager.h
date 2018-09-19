#pragma once

#include <memory>
#include "skip_list.h"
#include "timer.h"

class TimerManager final
{
public:
    TimerManager();
    virtual ~TimerManager() = default;

    static TimerManager * inst();

    void init();
    std::shared_ptr<Timer> create(int interval, int counter);
    int64_t time_now();
    int tick();

private:
    SkipList<int64_t, std::shared_ptr<Timer>, std::less<int64_t>, 32> _skip_list;
    int64_t _time_now;
    int64_t _next_tick_time;
};
