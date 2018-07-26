#pragma once

#include <list>
#include <typeinfo> // typeid
#include <type_traits> // is_pointer is_base_of
#include <memory> // shared_ptr
#include <functional> // function
#include "lua.hpp"
#include "error.h"
#include "lua_object.h"
#include "lua_bridge.h"

extern struct lua_State *lua_state;

void lua_port_init();
void lua_port_uninit();

template<typename T, typename...A>
inline int lua_push_x(lua_State *L, T value, A...args)
{
    int first = lua_push(L, value);
    int other = lua_push_x(L, args...);
    return first + other;
}

template<typename T>
inline int lua_push_x(lua_State *L, T value)
{
    return lua_push(L, value);
}

inline int lua_push_x(lua_State *L)
{
    return 0;
}

// class
extern int lua_class_index(lua_State *L);
extern int lua_class_newindex(lua_State *L);

template<class T>
int lua_gc_userdata(lua_State *L)
{
    int type = lua_type(L, 1);
    if (type != LUA_TUSERDATA)
        return 0;

    T *object = (T *)lua_touserdata(L, 1);
    object->~T();
    return 0;
}

template<class T>
inline void lua_class_setup_metatable(lua_State *L)
{
    static_assert(std::is_base_of<LuaObject, T>::value, "lua_class_setup_metatable failed, T must based on LuaObject");

    int top = lua_gettop(L);

    lua_getfield(L, top, "__index");
    if (lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, top, "__property");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, top, "__property");
    }

    lua_getfield(L, top, "__method");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, top, "__method");
    }

    lua_pushvalue(L, -2);
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, lua_class_index, 2);
    lua_setfield(L, top, "__index");

    lua_pushvalue(L, -2);
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, lua_class_newindex, 2);
    lua_setfield(L, top, "__newindex");

    lua_pushcfunction(L, &lua_gc_userdata< std::shared_ptr<LuaObject> >);
    lua_setfield(L, top, "__gc");

    lua_settop(L, top);
}

extern void lua_class_inherit_impl(lua_State *L, size_t n, ...);

template<typename...A>
inline void lua_class_inherit(lua_State *L)
{
    lua_class_inherit_impl(L, sizeof...(A), typeid(A).name()...);
}

extern void lua_class_get_metatable(lua_State *L, const char *type_id_name);
extern void lua_class_set_metatable(lua_State *L, const char *type_id_name);

template<class T, typename...A>
inline void lua_class_define(lua_State *L)
{
    int top = lua_gettop(L);
    const char *type_id_name = typeid(T).name();

    T::new_class(L);

    runtime_assert(lua_gettop(L) == top + 1, "class(%s).new_class error: top invalid", type_id_name);
    runtime_assert(lua_istable(L, -1), "class(%s).new_class error: should return table", type_id_name);

    lua_class_setup_metatable<T>(L);

    lua_pushvalue(L, -1);
    lua_class_set_metatable(L, type_id_name);

    lua_class_inherit<A...>(L);

    lua_settop(L, top);
}

template<class T>
inline void lua_push_ptr(lua_State *L, T *object)
{
    lua_newtable(L);

    lua_pushlightuserdata(L, (LuaObject *)object);
    lua_setfield(L, -2, "__ptr");

    lua_class_get_metatable(L, typeid(*object).name());
    lua_setmetatable(L, -2);
}

template<class T>
inline void lua_push_shared_ptr(lua_State *L, const std::shared_ptr<T> &object)
{
    void *userdata = lua_newuserdata(L, sizeof(std::shared_ptr<LuaObject>));
    auto lua_object = new (userdata) std::shared_ptr<LuaObject>();
    *lua_object = std::static_pointer_cast<LuaObject>(object);

    lua_class_get_metatable(L, typeid(*object.get()).name());
    lua_setmetatable(L, -2);
}

template<class T>
inline T * lua_to_ptr(lua_State *L, int index)
{
    if (index < 0)
        index += lua_gettop(L) + 1;

    int type = lua_type(L, index);
    switch (type)
    {
        case LUA_TNIL:
            return nullptr;

        case LUA_TUSERDATA:
        {
            std::shared_ptr<LuaObject> *object = (std::shared_ptr<LuaObject> *)lua_touserdata(L, index);
            return (T *)object->get();
        }

        case LUA_TTABLE:
        {
            lua_getfield(L, index, "__ptr");
            luaL_argcheck(L, lua_islightuserdata(L, -1), index, "__ptr is empty");

            LuaObject *object = (LuaObject *)lua_touserdata(L, -1);
            return (T *)object;
        }
    }

    luaL_argerror(L, index, "need an userdata or a table");
    return nullptr;
}

template<class T>
inline T * lua_to_ptr_safe(lua_State *L, int index)
{
    if (index < 0)
        index += lua_gettop(L) + 1;

    int type = lua_type(L, index);
    switch (type)
    {
        case LUA_TUSERDATA:
        {
            std::shared_ptr<LuaObject> *object = (std::shared_ptr<LuaObject> *)lua_touserdata(L, index);
            return (T *)object->get();
        }

        case LUA_TTABLE:
        {
            lua_getfield(L, index, "__ptr");
            if (lua_islightuserdata(L, -1))
                return (T *)(LuaObject *)lua_touserdata(L, -1);
        }
    }

    return nullptr;
}

template<class T>
inline std::shared_ptr<T> lua_to_shared_ptr(lua_State *L, int index)
{
    if (index < 0)
        index += lua_gettop(L) + 1;

    int type = lua_type(L, index);
    switch (type)
    {
        case LUA_TNIL:
            return nullptr;

        case LUA_TUSERDATA:
        {
            std::shared_ptr<LuaObject> *object = (std::shared_ptr<LuaObject> *)lua_touserdata(L, index);
            return std::static_pointer_cast<T>(*object);
        }

        case LUA_TTABLE:
        {
            lua_getfield(L, index, "__ptr");
            luaL_argcheck(L, lua_islightuserdata(L, -1), index, "__ptr is empty");

            LuaObject *object = (LuaObject *)lua_touserdata(L, -1);
            return std::static_pointer_cast<T>(object->shared_from_this());
        }
    }

    luaL_argerror(L, index, "need an userdata or a table");
    return nullptr;
}

template<class T>
inline std::shared_ptr<T> lua_to_shared_ptr_safe(lua_State *L, int index)
{
    if (index < 0)
        index += lua_gettop(L) + 1;

    int type = lua_type(L, index);
    switch (type)
    {
        case LUA_TUSERDATA:
        {
            std::shared_ptr<LuaObject> *object = (std::shared_ptr<LuaObject> *)lua_touserdata(L, index);
            return std::static_pointer_cast<T>(*object);
        }

        case LUA_TTABLE:
        {
            lua_getfield(L, index, "__ptr");
            if (lua_islightuserdata(L, -1))
            {
                LuaObject *object = (LuaObject *)lua_touserdata(L, -1);
                return std::static_pointer_cast<T>(object->shared_from_this());
            }
        }
    }

    return nullptr;
}

template<typename T>
struct LuaBridge
{
    static T to(lua_State* L, int index)
    {
        static_assert(std::is_pointer<T>::value, "lua_to failed, need a pointer");

        return lua_to_ptr<typename std::remove_pointer<T>::type>(L, index);
    }
    static int push(lua_State* L, T object)
    {
        static_assert(std::is_pointer<T>::value, "lua_push failed, need a pointer");
        static_assert(std::is_base_of<LuaObject, typename std::remove_pointer<T>::type>::value, "lua_push failed, need a LuaObject-based object");

        if (object == nullptr)
        {
            lua_pushnil(L);
            return 1;
        }

        LuaObject *lua_object = (LuaObject *)object;
        int ret = lua_object->lua_push_self(L);
        if (ret > 0)
            return ret;

        if (lua_object->get_luaref(L))
            return 1;

        lua_push_ptr(L, object);

        lua_object->retain_luaref(L);
        return 1;
    }
};

template<typename T>
struct LuaBridge< std::shared_ptr<T> >
{
    static std::shared_ptr<T> to(lua_State* L, int index)
    {
        if (index < 0)
            index += lua_gettop(L) + 1;

        int type = lua_type(L, index);
        switch (type)
        {
            case LUA_TNIL:
                return nullptr;

            case LUA_TUSERDATA:
            {
                std::shared_ptr<LuaObject> *object = (std::shared_ptr<LuaObject> *)lua_touserdata(L, index);
                return std::static_pointer_cast<T>(*object);
            }

            case LUA_TTABLE:
            {
                lua_getfield(L, index, "__ptr");
                luaL_argcheck(L, lua_islightuserdata(L, -1), index, "__ptr is empty");

                LuaObject *object = (LuaObject *)lua_touserdata(L, -1);
                return std::static_pointer_cast<T>(object->shared_from_this());
            }
        }

        luaL_argerror(L, index, "need an userdata or a table");
        return nullptr;
    }
    static int push(lua_State* L, const std::shared_ptr<T> &object)
    {
        static_assert(std::is_base_of<LuaObject, typename std::remove_pointer<T>::type>::value, "lua_push failed, need a LuaObject-based object");

        if (!object)
        {
            lua_pushnil(L);
            return 1;
        }

        int ret = object->lua_push_self(L);
        if (ret > 0)
            return ret;

        if (object->get_luaref(L))
            return 1;

        lua_push_shared_ptr(L, object);
        return 1;
    }
};

struct LuaObjectList : public LuaObject
{
    std::list<LuaObject *> list;

    virtual int lua_push_self(lua_State *L) override
    {
        int ret = 0;
        for (LuaObject *object : list)
            ret += lua_push(L, object);
        return ret;
    }
};

// functions
typedef std::function<int (lua_State *)> LuaFunction;

inline void set_lua_func_metatable(lua_State *L)
{
    int ret = luaL_newmetatable(L, "LuaFunction");
    if (ret == 1)
    {
        lua_pushcfunction(L, &lua_gc_userdata<LuaFunction>);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
}

template<typename...A, std::size_t...I>
inline void lua_push_std_func_userdata_impl(lua_State *L, std::function<void (A...)> f, std::index_sequence<I...> &&)
{
    void *userdata = lua_newuserdata(L, sizeof(LuaFunction));
    new (userdata) LuaFunction([f](lua_State *L){
        try
        {
            f(LuaBridge<A>::to(L, I + 1)...);
        }
        catch (const std::runtime_error &err)
        {
            luaL_error(L, "%s", err.what());
        }
        return 0;
    });
    set_lua_func_metatable(L);
}

template<typename R, typename...A, std::size_t...I>
inline void lua_push_std_func_userdata_impl(lua_State *L, std::function<R (A...)> f, std::index_sequence<I...> &&)
{
    void *userdata = lua_newuserdata(L, sizeof(LuaFunction));
    new (userdata) LuaFunction([f](lua_State *L){
        try
        {
            R r = f(LuaBridge<A>::to(L, I + 1)...);
            return LuaBridge<R>::push(L, r);
        }
        catch (const std::runtime_error &err)
        {
            luaL_error(L, "%s", err.what());
        }
        return 0;
    });
    set_lua_func_metatable(L);
}

template<typename R, typename...A>
inline void lua_push_std_func_userdata(lua_State *L, std::function<R (A...)> f)
{
    lua_push_std_func_userdata_impl(L, f, std::make_index_sequence<sizeof...(A)>());
}

template<typename T>
inline void lua_push_func_userdata(lua_State *L, T t);

template<typename R, typename...A>
inline void lua_push_func_userdata(lua_State *L, R (*f)(A...))
{
    lua_push_std_func_userdata(L, std::function<R (A...)>(f));
}

template<typename R, typename C, typename...A>
inline void lua_push_func_userdata(lua_State *L, R (C::*f)(A...))
{
    lua_push_std_func_userdata(L, std::function<R (C *, A...)>(std::mem_fn(f)));
}

template<typename R, typename C, typename...A>
inline void lua_push_func_userdata(lua_State *L, R (C::*f)(A...) const)
{
    lua_push_std_func_userdata(L, std::function<R (const C *, A...)>(std::mem_fn(f)));
}

template<typename C>
inline void lua_push_func_userdata(lua_State *L, int (C::*f)(lua_State *))
{
    void *userdata = lua_newuserdata(L, sizeof(LuaFunction));
    new (userdata) LuaFunction([f](lua_State *L){
        C *object = lua_to<C *>(L, 1);
        if (object != nullptr)
        {
            lua_remove(L, 1);
            try
            {
                return (object->*f)(L);
            }
            catch (const std::runtime_error &err)
            {
                luaL_error(L, "%s", err.what());
            }
        }
        return 0;
    });
    set_lua_func_metatable(L);
}

template<>
inline void lua_push_func_userdata(lua_State *L, LuaFunction f)
{
    void *userdata = lua_newuserdata(L, sizeof(LuaFunction));
    new (userdata) LuaFunction(f);
    set_lua_func_metatable(L);
}

extern int lua_closure(lua_State *L);

template<typename F>
inline void lua_push_function(lua_State *L, F f)
{
    lua_push_func_userdata(L, f);
    lua_pushcclosure(L, lua_closure, 1);
}

inline void lua_push_function(lua_State *L, int (*f)(lua_State *))
{
    lua_pushcfunction(L, f);
}

template<typename T>
inline T overload_func(T t)
{
    return t;
}

#define lua_new_class(L, Class) \
    typedef Class __Class; \
    if (0 == luaL_newmetatable(L, #Class)) return

#define lua_set_function(L, name, func) \
    lua_push_function(L, func); \
    lua_setfield(L, -2, name)
#define lua_set_function_sn(L, name, func, sign) lua_set_function(L, name, overload_func<sign>(func))

#define lua_function(L, func) lua_set_function(L, #func, func)
#define lua_function_sn(L, func, sign) lua_set_function_sn(L, #func, func, sign)

#define lua_push_method(L, method) lua_push_function(L, &__Class::method)
#define lua_push_method_sn(L, method, sign) lua_push_function(L, overload_func<sign>(&__Class::method))

#define lua_set_method(L, name, method) \
    lua_push_method(L, method); \
    lua_setfield(L, -2, name)
#define lua_set_method_sn(L, name, method, sign) \
    lua_push_method_sn(L, method, sign); \
    lua_setfield(L, -2, name)

#define lua_method(L, method) lua_set_method(L, #method, method)
#define lua_method_sn(L, method, sign) lua_set_method_sn(L, #method, method, sign)

#define lua_std_method(L, method) lua_set_method(L, #method, lua_##method)

#define lua_property_readonly(L, property) \
    lua_newtable(L); \
    { \
        lua_push_method(L, property); \
        lua_setfield(L, -2, "get"); \
    } \
    lua_setfield(L, -2, #property)

#define lua_lib(L, name) \
    do \
    { \
        int ret = lua_getglobal(L, name); \
        if (ret != LUA_TTABLE) \
        { \
            lua_pop(L, 1); \
            lua_newtable(L); \
            lua_pushvalue(L, -1); \
            lua_setglobal(L, name); \
        } \
    } while (false)


#define lua_property(L, property) \
    lua_newtable(L); \
    { \
        lua_push_method(L, property); \
        lua_setfield(L, -2, "get"); \
        lua_push_method(L, set_##property); \
        lua_setfield(L, -2, "set"); \
    } \
    lua_setfield(L, -2, #property)

inline int lua_btcall(lua_State *L, int nargs, int nresults)
{
    int top = lua_gettop(L);
    int start = top - nargs;

    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);

    lua_insert(L, start);
    int ret = lua_pcall(L, nargs, nresults, start);

    lua_remove(L, start);
    return ret;
}
