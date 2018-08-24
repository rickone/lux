#pragma once

#include "lua_port.h"

class Config final : public LuaObject
{
public:
    Config() = default;
    ~Config() = default;

    static void new_class(lua_State *L);

    bool daemon() const { return _daemon; }
    void set_daemon(bool daemon) { _daemon = daemon; }

private:
    bool    _daemon;
    int     _log_level;
    int     _listen_backlog;
    size_t  _socket_recv_buffer_init;
    size_t  _socket_send_buffer_init;
    size_t  _socket_send_buffer_max;
};

extern Config *config;
