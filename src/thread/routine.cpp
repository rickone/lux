#include "routine.h"
#include <sstream>
#include "log.h"
#include "lux_proto.h"
#include "routine_manager.h"

using namespace lux;

static std::string self_name() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

Routine::Routine() {
    auto L = get_lua_state();
    if (L) {
        _co = lua_newthread(L);
        _co_ref = luaL_ref(L, LUA_REGISTRYINDEX);

        printf("routine created: %p\n", this);
        fflush(stdout);
    }
}

Routine::~Routine() {
    if (_co) {
        if (_func_ref != LUA_NOREF) {
            luaL_unref(_co, LUA_REGISTRYINDEX, _func_ref);
            _func_ref = LUA_NOREF;
        }
    }

    auto L = get_lua_state();
    if (L) {
        if (_co_ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, _co_ref);
            _co_ref = LUA_NOREF;

            printf("routine removed: %p\n", this);
            fflush(stdout);
        }
    }
}

void Routine::new_class(lua_State* L) {
    lua_new_class(L, Routine);

    lua_newtable(L); {
    } lua_setfield(L, -2, "__method");

    lua_lib(L, "lux_core"); {
        lua_set_method(L, "create_routine", create);
        lua_set_function(L, "self_name", self_name);
    } lua_pop(L, 1);
}

int Routine::create(lua_State* L) {
    Routine* r = new Routine();
    if (!r->init(L)) {
        delete r;
        return 0;
    }

    RoutineManager::inst()->push_alive_routine(r);

    lua_pushboolean(L, 1);
    return 1;
}

bool Routine::init(lua_State* L) {
    if (!lua_isfunction(L, 1)) {
        log_error("need a function");
        return false;
    }

    lua_pushvalue(L, 1);
    lua_xmove(L, _co, 1);
    _func_ref = luaL_ref(_co, LUA_REGISTRYINDEX);

    lua_remove(L, 1);
    Proto pt;
    pt.lua_pack(L);
    _queue.push(pt.str().data(), pt.str().size());

    _status = kStatus_Running;
    return true;
}

int Routine::activate() {
    if (_status != kStatus_Running)
        return 0;

    auto node = _queue.pop();
    if (node == nullptr)
        return 0;

    resume_data(node->data, node->len);
    return 1;
}

void Routine::resume_data(const char* data, size_t len) {
    lua_rawgeti(_co, LUA_REGISTRYINDEX, _func_ref);

    Proto proto(data, len);
    int nargs = proto.lua_unpack(_co);

    int ret = lua_resume(_co, nullptr, nargs);
    if (ret == LUA_OK) {
        _status = kStatus_Finished;
    } else if (ret != LUA_YIELD) {
        _status = kStatus_Finished;
        log_error("error: %s", lua_tostring(_co, -1));
    }

    lua_settop(_co, 0);
}
