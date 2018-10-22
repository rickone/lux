#pragma once

#include "task.h"

namespace lux {

class LuaTask : public Task
{
public:
    LuaTask() = default;
    virtual ~LuaTask();

    virtual void on_exec(Proto &req, Proto &rsp) override;

private:
    lua_State *_state = nullptr;
};

} // lux
