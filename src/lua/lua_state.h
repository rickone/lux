#pragma once

#include "lua_port.h"

namespace lux {

class LuaState final
{
public:
    LuaState() = default;
    virtual ~LuaState();

    static LuaState *inst();

    void init();
    void gc();

private:
    void lua_path_init(lua_State *L);
    void lua_core_openlibs(lua_State *L);
    void lua_start_run(lua_State *L);

    lua_State *_state;
};

} // lux
