#pragma once

#include "ikcp.h"
#include "socket.h"
#include "buffer.h"

class SocketKcp : public Component
{
public:
    SocketKcp();
    virtual ~SocketKcp();

    static void new_class(lua_State *L);
    static std::shared_ptr<SocketKcp> create();

    void send(const std::string &str);
    int on_kcp_send(const char *data, size_t len);
    void on_recv(LuaObject *msg_object);
    void on_timer(Timer *timer);

    virtual void start(LuaObject *init_object) override;
    virtual void stop() noexcept override;

private:
    ikcpcb *_kcp;
    std::shared_ptr<Socket> _socket;
    Buffer _recv_buffer;
};
