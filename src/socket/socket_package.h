#pragma once

#include "socket.h"
#include "buffer.h"
#include "lua_port.h"

class SocketPackage : public Component
{
public:
    SocketPackage() = default;
    virtual ~SocketPackage() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<SocketPackage> create(int msg_type);

    void init(int msg_type);
    void on_recv(LuaObject *msg_object);

private:
    uint16_t _package_len;
};

struct LuaPackage : LuaObject
{
    std::string str;

    virtual int lua_push_self(lua_State *L) override
    {
        lua_pushlstring(L, str.data(), str.size());
        return 1;
    }
};
