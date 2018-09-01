#pragma once

#include "component.h"
#include "lua_port.h"

class LuaState : public Component
{
public:
    LuaState();
    virtual ~LuaState();

    static LuaState * inst();

    void init();
    void gc();

    virtual void start() override;
    virtual void stop() noexcept override;

private:
    void lua_path_init(lua_State *L);
    void lua_core_init(lua_State *L);
    void lua_core_openlibs(lua_State *L);
    void lua_start_run(lua_State *L);

    lua_State *_state;
};
