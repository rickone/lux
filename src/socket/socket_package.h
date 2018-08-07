#pragma once

#include "socket.h"
#include "buffer.h"
#include "lua_port.h"

class SocketPackage;
struct LuaPackage;

struct SocketPackageDelegate
{
    virtual void on_package_recv(SocketPackage *, LuaPackage *){}
};

class SocketPackage : public Component, public SocketDelegate, public Delegate<SocketPackageDelegate>
{
public:
    SocketPackage() = default;
    virtual ~SocketPackage() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<SocketPackage> create();

    int lua_send(lua_State *L);

    virtual void start() override;
    virtual void on_socket_recv(Socket *socket, Buffer *buffer) override;

private:
    std::shared_ptr<Socket> _socket;
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
