#pragma once

#include <memory>
#include "lua.hpp"

class LuaObject : public std::enable_shared_from_this<LuaObject>
{
public:
    LuaObject() = default;
    virtual ~LuaObject();

    bool get_luaref(lua_State *L);
    void retain_luaref(lua_State *L);
    void release_luaref(lua_State *L);

    virtual int lua_push_self(lua_State *L);
    virtual bool is_valid();

    int id() const { return _id; }
    void set_id(int id) { _id = id; }

private:
    int _id = 0;
    int _ref = LUA_NOREF;
};
