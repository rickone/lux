#include "task_manager.h"
#include "config.h"
#include "log.h"

TaskManager::~TaskManager()
{
    _run_flag = false;
    _cv.notify_all();

    for (auto &thread : _threads)
        thread.join();
}

TaskManager * TaskManager::inst()
{
    static TaskManager s_inst;
    return &s_inst;
}

void TaskManager::init()
{
    _run_flag = true;

    for (int i = 0; i < Config::env()->thread_num; ++i)
        _threads.emplace_back(std::bind(&TaskManager::task_func, this));
}

void TaskManager::commit(Task *task)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _tasks.push(task);
    }

    _cv.notify_one();
}

void TaskManager::task_func()
{
    puts("task_func start");

    while (_run_flag)
    {
        Task *task = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this] { return !_run_flag || (!_tasks.empty() && _tasks.front()->state() == kTaskState_Arranged); });
            if (!_run_flag)
                break;

            task = _tasks.front();
            _tasks.pop();
        }

        task->exec();
    }

    puts("task_func stop");
}
