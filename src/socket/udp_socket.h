#pragma once

#include <memory>
#include "socket.h"
#include "buffer.h"

class UdpSocket : public Socket
{
public:
    UdpSocket() = default;
    explicit UdpSocket(socket_t fd);
    virtual ~UdpSocket() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<UdpSocket> create(socket_t fd);
    static std::shared_ptr<UdpSocket> bind(const char *node, const char *service);
    static std::shared_ptr<UdpSocket> connect(const char *node, const char *service);

    void init_bind(const char *node, const char *service);
    void init_connect(const char *node, const char *service);
    void on_recv_buffer(Buffer *buffer);

    virtual void on_read(size_t len) override;

private:
    void on_recvfrom(size_t len);
    void on_recv(size_t len);

    Buffer _recv_buffer;
    Buffer _send_buffer;
    void (UdpSocket::*_on_read)(size_t len);

    sockaddr_storage _remote_sockaddr;
    socklen_t _remote_sockaddr_len;
};

struct LuaSockAddr : public LuaObject
{
    sockaddr *addr;
    socklen_t addrlen;

    virtual int lua_push_self(lua_State *L) override
    {
        lua_pushlstring(L, (const char *)addr, addrlen);
        return 1;
    }
};
