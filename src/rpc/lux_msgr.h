#pragma once

#include "socket.h"
#include "buffer.h"
#include "lua_port.h"

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
    void on_recv_package(const std::string &str);

    int lua_send(lua_State *L);
    
    virtual void start(LuaObject *init_object) override;
    virtual void stop() noexcept override;

private:
    bool _stream_mode;
    uint16_t _package_len;
    std::shared_ptr<Socket> _socket;
};
