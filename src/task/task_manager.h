#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "task.h"

class TaskManager final
{
public:
    TaskManager() = default;
    virtual ~TaskManager();

    static TaskManager * inst();

    void init();
    void commit(const std::shared_ptr<Task> &task);
    void task_func();

private:
    std::vector<std::thread> _threads;
    std::queue< std::shared_ptr<Task> > _tasks;
    std::mutex _mutex;
    std::condition_variable _cv;
    volatile bool _run_flag = false;
};
