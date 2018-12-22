#include "routine_manager.h"
#include "config.h"
#include "log.h"

using namespace lux;

RoutineManager::~RoutineManager() {
    _run_flag = false;

    for (auto &thread : _threads) {
        thread.join();
    }

    while (true) {
        auto r = pop_dead_routine();
        if (!r)
            break;

        delete r;
    }
}

RoutineManager* RoutineManager::inst() {
    static RoutineManager s_inst;
    return &s_inst;
}

void RoutineManager::init() {
    _run_flag = true;

    for (int i = 0; i < Config::env()->thread_num; ++i)
        _threads.emplace_back(std::bind(&RoutineManager::routine_func, this));
}

void RoutineManager::gc() {
    while (true) {
        auto r = pop_dead_routine();
        if (!r)
            break;

        delete r;
    }
}

void RoutineManager::routine_func() {
    log_info("routine_func start");

    std::list<Routine*> local_queue;

    while (_run_flag) {
        auto r = pop_alive_routine();
        if (r)
            local_queue.push_back(r);

        auto it = local_queue.begin();
        auto it_end = local_queue.end();
        int n = 0;
        while (it != it_end) {
            auto r = *it;
            n += r->activate();

            if (r->status() == Routine::kStatus_Finished) {
                local_queue.erase(it++);
                push_dead_routine(r);
            } else {
                ++it;
            }
        }

        if (n == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    for (auto r : local_queue) {
        push_dead_routine(r);
    }

    log_info("routine_func exit");
}

void RoutineManager::push_alive_routine(Routine* routine) {
    _alive_routines.push((const char*)&routine, sizeof(routine));
}

Routine* RoutineManager::pop_alive_routine() {
    auto node = _alive_routines.pop();
    if (node == nullptr)
        return nullptr;

    return *(Routine**)node->data;
}

void RoutineManager::push_dead_routine(Routine* routine) {
    _dead_routines.push((const char*)&routine, sizeof(routine));
}

Routine* RoutineManager::pop_dead_routine() {
    auto node = _dead_routines.pop();
    if (node == nullptr)
        return nullptr;

    return *(Routine**)node->data;
}
