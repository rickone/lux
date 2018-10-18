#include "task.h"
#include "task_manager.h"

void Task::request(const LuxProto &req)
{
    runtime_assert(_state == kTaskState_Idle, "task cant invoke");

    _req = req;
    set_state(kTaskState_Arranged);
    TaskManager::inst()->commit(this);
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
    int a = req.unpack<int>();
    int b = req.unpack<int>();
    int r = a + b;
    std::string str("task result: ");
    str += std::to_string(a);
    str.append(" + ");
    str += std::to_string(b);
    str.append(" = ");
    str += std::to_string(r);
    rsp.pack(str);
}
