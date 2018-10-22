#include "task_thread_pool.h"
#include "config.h"
#include "log.h"

using namespace lux;

TaskThreadPool::~TaskThreadPool()
{
    _run_flag = false;
    _cv.notify_all();

    for (auto &thread : _threads)
        thread.join();

    while (!_tasks.empty())
    {
        auto &task = _tasks.front();
        task->set_state(kTaskState_Finished);
        _tasks.pop();
    }
}

TaskThreadPool * TaskThreadPool::inst()
{
    static TaskThreadPool s_inst;
    return &s_inst;
}

void TaskThreadPool::init()
{
    _run_flag = true;

    for (int i = 0; i < Config::env()->thread_num; ++i)
        _threads.emplace_back(std::bind(&TaskThreadPool::task_func, this));
}

void TaskThreadPool::commit(const std::shared_ptr<Task> &task)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _tasks.push(task);
    }

    _cv.notify_one();
}

void TaskThreadPool::task_func()
{
    while (_run_flag)
    {
        std::shared_ptr<Task> task;
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
}
