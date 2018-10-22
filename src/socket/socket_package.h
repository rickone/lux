#pragma once

#include "socket.h"
#include "buffer.h"

namespace lux {

class SocketPackage : public Object
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
    uint16_t _header = 0;
    size_t _package_len = 0;
    Buffer _package_buffer;
};

} // lux
