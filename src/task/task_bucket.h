#pragma once

#include <memory>
#include <queue>
#include <list>
#include "task.h"
#include "callback.h"
#include "timer.h"

class TaskBucket : public LuaObject
{
public:
    TaskBucket() = default;
    virtual ~TaskBucket() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<TaskBucket> create(int num);

    void init();
    void clear();
    void add(const std::shared_ptr<Task> &task);
    void request(const LuxProto &pt);
    void on_timer();
    int lua_request(lua_State *L);

    virtual bool is_valid() override { return !_clear_flag || !_busy_tasks.empty(); }

    def_lua_callback(on_respond, TaskBucket *, LuxProto *)

private:
    std::list<LuxProto> _pending_reqs;
    std::list< std::shared_ptr<Task> > _idle_tasks;
    std::list< std::shared_ptr<Task> > _busy_tasks;
    std::shared_ptr<Timer> _timer;
    bool _clear_flag = false;
};
