#pragma once

#include "ikcp.h"
#include "socket.h"

class SocketKcp;

struct SocketKcpDelegate
{
    virtual void on_kcp_recv(SocketKcp *, Buffer *){}
};

class SocketKcp : public Component, public SocketDelegate, public Delegate<SocketKcpDelegate>
{
public:
    SocketKcp() = default;
    virtual ~SocketKcp();

    static void new_class(lua_State *L);
    static std::shared_ptr<SocketKcp> create();

    int socket_send(const char *data, size_t len);
    void on_timer(Timer *timer);

    void send(const char *data, size_t len);
    int lua_send(lua_State *L);

    virtual void start() override;
    virtual void stop() noexcept override;
    virtual void on_socket_recv(Socket *socket, Buffer *buffer) override;

private:
    std::shared_ptr<Socket> _socket;
    ikcpcb *_kcp;
    Buffer _recv_buffer;
};
