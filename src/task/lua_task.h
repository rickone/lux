#pragma once

#include "task.h"

namespace lux {

class LuaTask : public Task
{
public:
    LuaTask() = default;
    virtual ~LuaTask();

    virtual void on_exec(LuxProto &req, LuxProto &rsp) override;

private:
    lua_State *_lua_state = nullptr;
};

} // lux
