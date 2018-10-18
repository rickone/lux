#pragma once

#include <atomic>
#include "lux_proto.h"
#include "error.h"

enum TaskState
{
    kTaskState_Idle,
    kTaskState_Arranged,
    kTaskState_InProgress,
    kTaskState_Finished,
};

class Task
{
public:
    Task() = default;
    virtual ~Task() = default;

    void request(const LuxProto &pt);
    void exec();

    virtual void on_exec();

    TaskState state() { return _state.load(std::memory_order_acquire); }
    void set_state(TaskState state) { _state.store(state, std::memory_order_release); }

    const LuxProto & respond() const { return _rsp; }

protected:
    std::atomic<TaskState> _state = { kTaskState_Idle };
    LuxProto _req;
    LuxProto _rsp;
};
