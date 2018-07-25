#pragma once

#include <memory>
#include <chrono>
#include "skip_list.h"
#include "timer.h"

class TimerManager final
{
public:
    typedef std::function<void (int)> Sleep;

    TimerManager();
    ~TimerManager() = default;

    std::shared_ptr<Timer> create(int interval, int counter);
    int64_t time_now();
    void set_sleep(const Sleep &sleep);
    void update();

private:
    SkipList<int64_t, std::shared_ptr<Timer>, std::less<int64_t>, 32> _skip_list;
    std::function<void (int timeout)> _sleep;
    std::chrono::time_point<std::chrono::steady_clock> _start_time;
    int64_t _time_now;
};

extern TimerManager *timer_manager;
