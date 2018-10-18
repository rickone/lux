#include "task.h"
#include "task_manager.h"

void Task::request(const LuxProto &req)
{
    runtime_assert(_state == kTaskState_Idle, "task cant invoke");

    _req = req;
    set_state(kTaskState_Arranged);
    TaskManager::inst()->commit(shared_from_this());
}

void Task::exec()
{
    set_state(kTaskState_InProgress);
    _rsp.clear();
    on_exec(_req, _rsp);
    set_state(kTaskState_Finished);
}

void Task::on_exec(LuxProto &req, LuxProto &rsp)
{
}
