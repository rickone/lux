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
    std::shared_ptr<Timer> create(unsigned int interval, int counter, unsigned int delay);
    uint64_t time_now();
    int tick();

private:
    SkipList<int64_t, std::shared_ptr<Timer>, std::less<int64_t>, 32> _skip_list;
    uint64_t _time_now;
    uint64_t _next_tick_time;
};
