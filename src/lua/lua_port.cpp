#include "lua_port.h"
#include <cassert>

struct lua_State *lua_state = nullptr;

int lua_class_index(lua_State *L)
{
    int property = lua_upvalueindex(1);
    int method = lua_upvalueindex(2);

    lua_pushvalue(L, 2);
    lua_rawget(L, property);
    if (lua_istable(L, -1))
    {
        lua_pushstring(L, "get");
        lua_rawget(L, -2);

        luaL_checktype(L, -1, LUA_TFUNCTION);
        lua_pushvalue(L, 1);
        if (LUA_OK != lua_pcall(L, 1, 1, 0))
            luaL_error(L, "property.get error: %s", lua_tostring(L, -1));

        return 1;
    }

    lua_pushvalue(L, 2);
    lua_rawget(L, method);
    return 1;
}

int lua_class_newindex(lua_State *L)
{
    int property = lua_upvalueindex(1);
    int method = lua_upvalueindex(2);

    lua_pushvalue(L, 2);
    lua_rawget(L, property);
    if (lua_istable(L, -1))
    {
        lua_pushstring(L, "set");
        lua_rawget(L, -2);

        luaL_checktype(L, -1, LUA_TFUNCTION);
        lua_pushvalue(L, 1);
        lua_pushvalue(L, 3);

        if (LUA_OK != lua_pcall(L, 2, 0, 0))
            luaL_error(L, "property.set error: %s", lua_tostring(L, -1));

        return 0;
    }

    lua_pushvalue(L, 2);
    lua_rawget(L, method);
    if (!lua_isnil(L, -1))
        luaL_error(L, "property.set error: field '%s' is a method", lua_tostring(L, 2));

    if (lua_isuserdata(L, 1))
        luaL_error(L, "property.set error: cant set '%s' to an userdata", lua_tostring(L, 2));

    lua_settop(L, 3);
    lua_rawset(L, 1);
    return 0;
}

// copy the field from -1 to -2
static void lua_copy_metatable_field(lua_State *L, const char *field)
{
    int top = lua_gettop(L);

    lua_getfield(L, top - 1, field);
    runtime_assert(lua_istable(L, -1), "lua_copy_metatable_field(%s) error: not exists", field);

    int target = top + 1;

    lua_getfield(L, top, field);
    runtime_assert(lua_istable(L, -1), "lua_copy_metatable_field(%s) error: not exists", field);

    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        lua_pushvalue(L, -2);
        int ret = lua_rawget(L, target);
        if (ret != LUA_TNIL)
        {
            lua_pop(L, 2);
            continue;
        }

        lua_pop(L, 1);
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_rawset(L, target);
    }

    lua_settop(L, top);
}

static void lua_class_inherit_impl_one(lua_State *L, const char *type_id_name)
{
    int top = lua_gettop(L);

    lua_class_get_metatable(L, type_id_name);

    lua_copy_metatable_field(L, "__property");
    lua_copy_metatable_field(L, "__method");
  
    lua_settop(L, top);
}

void lua_class_inherit_impl(lua_State *L, size_t count, ...)
{
    va_list args;
    va_start(args, count);

    for(size_t i = 0; i < count; i++)          
    {
        const char *type_id_name = va_arg(args, const char *);
        lua_class_inherit_impl_one(L, type_id_name);
    }

    va_end(args);
}

void lua_class_get_metatable(lua_State *L, const char *type_id_name)
{
    int ret = lua_getglobal(L, "class_meta");
    if (ret != LUA_TTABLE)
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "class_meta");
    }

    lua_getfield(L, -1, type_id_name);
    lua_remove(L, -2);

    if (!lua_istable(L, -1))
        luaL_error(L, "class(%s).lua_class_get_metatable error: metatable not exists", type_id_name);
}

void lua_class_set_metatable(lua_State *L, const char *type_id_name)
{
    int ret = lua_getglobal(L, "class_meta");
    if (ret != LUA_TTABLE)
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "class_meta");
    }

    lua_insert(L, -2);
    lua_setfield(L, -2, type_id_name);
    lua_pop(L, 1);
}

int lua_closure(lua_State *L)
{
    LuaFunction *func = (LuaFunction *)lua_touserdata(L, lua_upvalueindex(1));
    return (*func)(L);
}
