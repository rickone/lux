#pragma once

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

    void init();
    void add(Task *task);
    void on_timer();

    def_lua_callback(on_respond, LuxProto *)

private:
    std::queue<LuxProto> _pending_reqs;
    std::list<Task *> _idle_tasks;
    std::list<Task *> _busy_tasks;
    std::shared_ptr<Timer> _timer;
};
