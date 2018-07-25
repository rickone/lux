#pragma once

class LuaObject : public std::enable_shared_from_this<LuaObject>
{
public:
    LuaObject();
    virtual ~LuaObject();

    bool get_luaref(lua_State *L);
    void retain_luaref(lua_State *L);
    void release_luaref(lua_State *L);

    virtual int lua_push_self(lua_State *L);

private:
    int _ref;
};
