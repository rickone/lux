#include "task_bucket.h"
#include "lua_task.h"

void TaskBucket::new_class(lua_State *L)
{
    lua_new_class(L, TaskBucket);

    lua_newtable(L);
    {
        lua_callback(L, on_respond);
    }
    lua_setfield(L, -2, "__property");

    lua_newtable(L);
    {
        lua_std_method(L, request);
    }
    lua_setfield(L, -2, "__method");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_task_bucket", lua_create);
    }
    lua_pop(L, 1);
}

int TaskBucket::lua_create(lua_State *L)
{
    const char *task_file = luaL_checkstring(L, 1);
    int num = (int)luaL_checkinteger(L, 2);

    auto task_bucket = TaskBucket::create<LuaTask>(num, task_file);
    lua_push(L, task_bucket);
    return 1;
}

void TaskBucket::init()
{
    _timer = Timer::create(10);
    _timer->on_timer.set(this, &TaskBucket::on_timer);
}

void TaskBucket::clear()
{
    _clear_flag = true;
}

void TaskBucket::add(const std::shared_ptr<Task> &task)
{
    _idle_tasks.push_back(task);
}

void TaskBucket::request(const LuxProto &pt)
{
    if (_clear_flag)
        return;

    if (!_pending_reqs.empty())
    {
        _pending_reqs.push_back(pt);
        return;
    }

    if (_idle_tasks.empty())
    {
        _pending_reqs.push_back(pt);
        return;
    }

    auto &task = _idle_tasks.front();
    task->request(pt);

    _busy_tasks.push_back(task);
    _idle_tasks.pop_front();
}

void TaskBucket::on_timer()
{
    auto it = _busy_tasks.begin();
    auto it_end = _busy_tasks.end();
    while (it != it_end)
    {
        auto &task = *it;
        if (task->state() != kTaskState_Finished)
        {
            ++it;
            continue;
        }

        LuxProto rsp(task->respond());
        on_respond(this, &rsp);
        task->set_state(kTaskState_Idle);

        _idle_tasks.push_back(task);
        _busy_tasks.erase(it++);
    }

    if (_clear_flag)
        return;

    while (!_pending_reqs.empty() && !_idle_tasks.empty())
    {
        LuxProto &pt = _pending_reqs.front();
        auto &task = _idle_tasks.front();

        task->request(pt);

        _pending_reqs.pop_front();
        _busy_tasks.push_back(task);
        _idle_tasks.pop_front();
    }
}

int TaskBucket::lua_request(lua_State *L)
{
    LuxProto pt;
    pt.lua_pack(L);

    request(pt);
    return 0;
}
