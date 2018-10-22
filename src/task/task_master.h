#pragma once

#include <queue>
#include <list>
#include "task.h"
#include "callback.h"
#include "timer.h"
#include "object_manager.h"

namespace lux {

class TaskMaster : public Object
{
public:
    TaskMaster() = default;
    virtual ~TaskMaster() = default;

    static void new_class(lua_State *L);
    static int lua_create(lua_State *L);

    void init();
    void clear();
    void add(const std::shared_ptr<Task> &task);
    void request(const LuxProto &pt);
    void request_all(const LuxProto &pt);
    void on_timer();
    int lua_request(lua_State *L);

    virtual bool is_valid() override { return !_clear_flag || !_busy_tasks.empty(); }

    def_lua_callback(on_respond, TaskMaster *, LuxProto *)

    template<typename T, typename...A>
    static std::shared_ptr<TaskMaster> create(int num, A...args);

private:
    std::list<LuxProto> _pending_reqs;
    std::list< std::shared_ptr<Task> > _idle_tasks;
    std::list< std::shared_ptr<Task> > _busy_tasks;
    std::shared_ptr<Timer> _timer;
    bool _clear_flag = false;
};

template<typename T, typename...A>
std::shared_ptr<TaskMaster> TaskMaster::create(int num, A...args)
{
    static_assert(std::is_base_of<Task, T>::value, "TaskBucket::create failed, need a Task-based type");

    auto task_master = ObjectManager::inst()->create<TaskMaster>();
    task_master->init();
    for (int i = 0; i < num; ++i)
        task_master->add(std::make_shared<T>(args...));

    return task_master;
}

} // lux
