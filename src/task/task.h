#pragma once

#include <memory>
#include <atomic>
#include "lux_proto.h"
#include "error.h"

namespace lux {

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

    void request(const Proto &req);
    void exec();

    virtual void on_exec(Proto &req, Proto &rsp);

    TaskState state() { return _state.load(std::memory_order_acquire); }
    void set_state(TaskState state) { _state.store(state, std::memory_order_release); }

    const Proto &respond() const { return _rsp; }

private:
    std::atomic<TaskState> _state = { kTaskState_Idle };
    Proto _req;
    Proto _rsp;
};

} // lux
