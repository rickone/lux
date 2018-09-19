#pragma once

#include "ikcp.h"
#include "lua_port.h"
#include "timer.h"
#include "buffer.h"

class SocketKcp : public LuaObject
{
public:
    SocketKcp() = default;
    virtual ~SocketKcp();

    static void new_class(lua_State *L);
    static std::shared_ptr<SocketKcp> create();

    void init();
    void recv(const char *data, size_t len);
    void send(const char *data, size_t len);
    void on_timer(Timer *timer);

    int lua_recv(lua_State *L);
    int lua_send(lua_State *L);

    def_lua_callback(on_recv, SocketKcp *, Buffer *)
    def_lua_callback(on_send, RawData *)

private:
    ikcpcb *_kcp;
    Buffer _recv_buffer;
};
