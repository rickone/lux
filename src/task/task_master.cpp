#include "task_master.h"
#include "lua_task.h"

using namespace lux;

void TaskMaster::new_class(lua_State *L)
{
    lua_new_class(L, TaskMaster);

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
        lua_set_method(L, "create_task_master", lua_create);
    }
    lua_pop(L, 1);
}

int TaskMaster::lua_create(lua_State *L)
{
    int num = (int)luaL_checkinteger(L, 1);

    Proto pt;
    int top = lua_gettop(L);
    for (int i = 2; i <= top; ++i)
        pt.pack_lua_object(L, i);

    auto task_master = TaskMaster::create<LuaTask>(num);
    task_master->request_all(pt);

    lua_push(L, task_master);
    return 1;
}

void TaskMaster::init()
{
    _timer = Timer::create(10);
    _timer->on_timer.set(this, &TaskMaster::on_timer);
}

void TaskMaster::clear()
{
    _clear_flag = true;
}

void TaskMaster::add(const std::shared_ptr<Task> &task)
{
    _idle_tasks.push_back(task);
}

void TaskMaster::request(const Proto &pt)
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

void TaskMaster::request_all(const Proto &pt)
{
    if (_clear_flag)
        return;

    for (auto &task : _idle_tasks)
    {
        task->request(pt);
        _busy_tasks.push_back(task);
    }
    _idle_tasks.clear();
}

void TaskMaster::on_timer()
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

        Proto rsp(task->respond());
        on_respond(this, &rsp);
        task->set_state(kTaskState_Idle);

        _idle_tasks.push_back(task);
        _busy_tasks.erase(it++);
    }

    if (_clear_flag)
        return;

    while (!_pending_reqs.empty() && !_idle_tasks.empty())
    {
        Proto &pt = _pending_reqs.front();
        auto &task = _idle_tasks.front();

        task->request(pt);

        _pending_reqs.pop_front();
        _busy_tasks.push_back(task);
        _idle_tasks.pop_front();
    }
}

int TaskMaster::lua_request(lua_State *L)
{
    Proto pt;
    pt.lua_pack(L);

    request(pt);
    return 0;
}
