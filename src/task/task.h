#pragma once

#include <memory>
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

class Task : public std::enable_shared_from_this<Task>
{
public:
    Task() = default;
    virtual ~Task() = default;

    void request(const LuxProto &req);
    void exec();

    virtual void on_exec(LuxProto &req, LuxProto &rsp);

    TaskState state() { return _state.load(std::memory_order_acquire); }
    void set_state(TaskState state) { _state.store(state, std::memory_order_release); }

    const LuxProto & respond() const { return _rsp; }

private:
    std::atomic<TaskState> _state = { kTaskState_Idle };
    LuxProto _req;
    LuxProto _rsp;
};
