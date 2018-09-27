#pragma once

#include "socket.h"
#include "buffer.h"

class SocketPackage : public LuaObject
{
public:
    SocketPackage() = default;
    virtual ~SocketPackage() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<SocketPackage> create();

    void recv(Buffer *buffer);
    void send(const char *data, size_t len);

    int lua_recv(lua_State *L);
    int lua_send(lua_State *L);

    def_lua_callback(on_recv, Buffer *)
    def_lua_callback(on_send, RawBuffer *, size_t)

private:
    uint16_t _header;
    size_t _package_len;
    Buffer _package_buffer;
};

