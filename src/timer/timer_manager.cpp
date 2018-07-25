#include "timer_manager.h"
#include <cassert>
#include <thread>

TimerManager *timer_manager = nullptr;

using namespace std::chrono;

TimerManager::TimerManager() : _skip_list(2), _sleep([](int timeout){ std::this_thread::sleep_for(std::chrono::milliseconds(timeout)); }), _time_now()
{
    assert(timer_manager == nullptr);
    timer_manager = this;

    _start_time = steady_clock::now();
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

void TimerManager::set_sleep(const Sleep &sleep)
{
    _sleep = sleep;
}

void TimerManager::update()
{
    auto now = steady_clock::now();
    auto dur = duration_cast<milliseconds>(now - _start_time);
    _time_now = dur.count();

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
    if (first_node)
        _sleep((int)(first_node->get_key() - _time_now));
    else
        _sleep(100);
}
