#pragma once

#include "lua_port.h"
#include "error.h"

namespace lux {

template<typename... A>
class Callback
{
public:
    Callback() : _object(), _func(), _lua_ref(LUA_NOREF)
    {
    }

    ~Callback()
    {
        auto L = get_lua_state();
        if (L)
            unref_lua_func(L);
    }

    void clear()
    {
        _object.reset();
        _func = nullptr;
        auto L = get_lua_state();
        if (L)
            unref_lua_func(L);
    }

    template<typename T>
    void set(T *object, const std::function<void (T *, A...)> &func)
    {
        static_assert(std::is_base_of<Object, T>::value, "Callback<T>.set failed, T must based on Object");

        _object = object->shared_from_this();
        _func = [func](Object *object, A...args){
            func((T *)object, args...);
        };
    }

    template<typename T>
    void set(T *object, void (T::*func)(A...))
    {
        std::function<void (T *, A...)> mfn = std::mem_fn(func);
        set(object, mfn);
    }

    void ref_lua_func(lua_State *L)
    {
        luaL_checktype(L, -1, LUA_TFUNCTION);

        unref_lua_func(L);
        _lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    void unref_lua_func(lua_State *L)
    {
        if (!L)
            return;

        if (_lua_ref != LUA_NOREF)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, _lua_ref);
            _lua_ref = LUA_NOREF;
        }
    }

    int get_lua_func(lua_State *L)
    {
        if (_lua_ref == LUA_NOREF)
            return 0;

        lua_rawgeti(L, LUA_REGISTRYINDEX, _lua_ref);
        return 1;
    }

    void operator()(A... args)
    {
        auto object = _object.lock();
        if (object)
            _func(object.get(), args...);

        if (_lua_ref == LUA_NOREF)
            return;

        auto L = get_lua_state();
        if (!L)
            return;

        int top = lua_gettop(L);

        lua_rawgeti(L, LUA_REGISTRYINDEX, _lua_ref);
        lua_push_x(L, args...);

        int ret = lua_btcall(L, sizeof...(args), 0);
        if (ret != LUA_OK)
        {
            std::string err(lua_tostring(L, -1));
            lua_settop(L, top);
            throw_error(std::runtime_error, "[Lua] %s", err.c_str());
        }
    }

private:
    std::weak_ptr<Object> _object;
    std::function<void (Object *, A...)> _func;
    int _lua_ref;
};

} // lux

#define def_lua_callback(callback, ...) \
    lux::Callback<__VA_ARGS__> callback; \
    int lua_get_##callback(lua_State *L) { return callback.get_lua_func(L); } \
    int lua_set_##callback(lua_State *L) { callback.ref_lua_func(L); return 0; }

#define lua_callback(L, callback) \
    lua_newtable(L); \
    { \
        lua_set_method(L, "get", lua_get_##callback); \
        lua_set_method(L, "set", lua_set_##callback); \
    } \
    lua_setfield(L, -2, #callback)
