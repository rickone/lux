#include "task_bucket.h"

void TaskBucket::new_class(lua_State *L)
{
    lua_new_class(L, TaskBucket);

    lua_newtable(L);
    {
        lua_callback(L, on_respond);
    }
    lua_setfield(L, -2, "__property");
}

void TaskBucket::init()
{
    _timer = Timer::create(30);
    _timer->on_timer.set(this, &TaskBucket::on_timer);
}

void TaskBucket::add(Task *task)
{
    _idle_tasks.push_back(task);
}

void TaskBucket::on_timer()
{
    for (auto it = _busy_tasks.begin(); it != _busy_tasks.end(); )
    {
        Task *task = *it;
        if (task->state() == kTaskState_Finished)
        {
            LuxProto rsp(task->respond());
            on_respond(&rsp);
            task->set_state(kTaskState_Idle);

            _busy_tasks.erase(it++);
            _idle_tasks.push_back(task);
        }
        else
        {
            ++it;
        }
    }
}
