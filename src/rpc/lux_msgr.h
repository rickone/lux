#pragma once

#include "socket.h"
#include "buffer.h"
#include "lux_proto.h"

class Messenger : public Component
{
public:
    Messenger() = default;
    virtual ~Messenger() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<Messenger> create(int msg_type, bool stream_mode);

    void init(int msg_type, bool stream_mode);
    void on_recv_stream(LuaObject *msg_object);
    void on_recv_dgram(LuaObject *msg_object);
    void on_recv_package(Buffer *buffer, size_t len);

    int lua_send(lua_State *L);
    
    virtual void start(LuaObject *init_object) override;
    virtual void stop() noexcept override;

private:
    uint16_t _header_len;
    std::shared_ptr<Socket> _socket;
    Buffer _buffer;
};
