#include "task.h"
#include "task_manager.h"

void Task::request(const LuxProto &pt)
{
    runtime_assert(_state == kTaskState_Idle, "task cant invoke");

    _req = pt;
    set_state(kTaskState_Arranged);
    TaskManager::inst()->commit(this);
}

void Task::exec()
{
    set_state(kTaskState_InProgress);
    _rsp.clear();
    on_exec();
    set_state(kTaskState_Finished);
}

void Task::on_exec()
{
    int a = _req.unpack<int>();
    int b = _req.unpack<int>();
    int r = a + b;
    std::string str("task result: ");
    str += std::to_string(a);
    str.append(" + ");
    str += std::to_string(b);
    str.append(" = ");
    str += std::to_string(r);
    _rsp.pack(str);
}
