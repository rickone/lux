#pragma once

#include <vector>
#include <thread>
#include "routine.h"
#include "queue.h"
#include "processor.h"

namespace lux {

class RoutineManager {
public:
    RoutineManager() = default;
    virtual ~RoutineManager();

    static RoutineManager* inst();

    void init();
    void gc();
    void routine_func();

    void push_alive_routine(Routine* routine);
    Routine* pop_alive_routine();

    void push_dead_routine(Routine* routine);
    Routine* pop_dead_routine();

private:
    Queue _alive_routines;
    Queue _dead_routines;

    std::vector<std::thread> _threads;
    volatile bool _run_flag = false;
};

} // lux
