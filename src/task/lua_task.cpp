#include "lua_task.h"

LuaTask::LuaTask(const char *file_path)
{
    lua_State *L = luaL_newstate();
    _lua_state = L;

    luaL_openlibs(L);

    int ret = luaL_loadfile(L, file_path);
    if (ret != LUA_OK)
        luaL_error(L, "loadfile(%s) error: %s", file_path, lua_tostring(L, -1));

    lua_call(L, 0, 0);
}

LuaTask::~LuaTask()
{
    if (_lua_state)
    {
        lua_close(_lua_state);
        _lua_state = nullptr;
    }
}

void LuaTask::on_exec(LuxProto &req, LuxProto &rsp)
{
    std::string func_name = req.unpack<std::string>();

    lua_State *L = _lua_state;
    int top = lua_gettop(L);

    int ret = lua_getglobal(L, func_name.c_str());
    if (ret != LUA_TFUNCTION)
    {
        fprintf(stderr, "[LuaTask] %s\n", lua_tostring(L, -1));
        lua_settop(L, top);
        return;
    }

    int nargs = req.lua_unpack(L);
    ret = lua_btcall(L, nargs, LUA_MULTRET);
    if (ret != LUA_OK)
    {
        fprintf(stderr, "[LuaTask] %s\n", lua_tostring(L, -1));
        lua_settop(L, top);
        return;
    }

    int nresults = lua_gettop(L) - top;
    for (int i = 1; i <= nresults; ++i)
    {
        rsp.pack_lua_object(L, top + i);
    }
}
