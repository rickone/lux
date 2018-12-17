#include "routine.h"
#include "log.h"

using namespace lux;

Routine::~Routine() {
    auto L = get_lua_state();
    if (L) {
        if (_co_ref != LUA_NOREF)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, _co_ref);
            _co_ref = LUA_NOREF;
        }
    }
}

void Routine::new_class(lua_State* L) {
    lua_new_class(L, Routine);

    lua_newtable(L); {
        lua_method(L, load);
        lua_method(L, run);
        lua_method(L, resume);
    } lua_setfield(L, -2, "__method");

    lua_lib(L, "lux_core"); {
        lua_set_method(L, "create_routine", create);
    } lua_pop(L, 1);
}

int Routine::create(lua_State* L) {
    auto routine = std::make_shared<Routine>();
    routine->init(L);
    return lua_push(L, routine);
}

int Routine::init(lua_State* L) {
    _co = lua_newthread(L);
    _co_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

int Routine::load(lua_State *L) {
    const char* filename = luaL_checkstring(L, 1);
    int ret = luaL_loadfile(_co, filename);
    if (ret != LUA_OK) {
        lua_xmove(_co, L, 1);
        return lua_error(L);
    }

    _func_ref = luaL_ref(_co, LUA_REGISTRYINDEX);
    return 0;
}

int Routine::run(lua_State* L) {
    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1);
        lua_xmove(L, _co, 1);
        _func_ref = luaL_ref(_co, LUA_REGISTRYINDEX);
    }

    lua_remove(L, 1);
    return resume(L);
}

int Routine::resume(lua_State* L) {
    if (_func_ref == LUA_NOREF) {
        return luaL_error(L, "no func");
    }

    lua_rawgeti(_co, LUA_REGISTRYINDEX, _func_ref);

    int nargs = lua_gettop(L);
    lua_xmove(L, _co, nargs);

    int ret = lua_resume(_co, L, nargs);
    if (ret == LUA_YIELD) {
        log_info("yield");
    } else if (ret == LUA_OK) {
        log_info("finish");
    } else {
        log_error("error: %s", lua_tostring(_co, -1));
        return 0;
    }

    int nresults = lua_gettop(_co);
    lua_xmove(_co, L, nresults);
    return nresults;
}
