#pragma once

#include <memory>
#include "lua_port.h"

namespace lux {

enum RoutineStatus {
    kRoutineStatus_Suspended,
    kRoutineStatus_DataReady,
    kRoutineStatus_Finished,
};

class Routine : public Object {
public:
    Routine() = default;
    virtual ~Routine();

    static void new_class(lua_State *L);
    static int create(lua_State* L);

    int init(lua_State* L);
    int load(lua_State* L);
    int run(lua_State* L);
    int resume(lua_State* L);

private:
    lua_State* _co = nullptr;
    int _co_ref = LUA_NOREF;
    int _func_ref = LUA_NOREF;
};

} // lux
