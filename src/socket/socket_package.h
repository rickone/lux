#pragma once

#include "socket.h"
#include "buffer.h"
#include "lua_port.h"

struct LuaPackage;

class SocketPackage : public Component
{
public:
    SocketPackage() = default;
    virtual ~SocketPackage() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<SocketPackage> create();
    void on_socket_recv(Socket *socket, Buffer *buffer);

    int lua_send(lua_State *L);

    virtual void start() override;

    template<typename T, typename F>
    void set_callback(T *object, F func) { _callback.set(object, func); }

private:
    std::shared_ptr<Socket> _socket;
    uint16_t _package_len;
    Callback<SocketPackage *, LuaPackage *> _callback;
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
