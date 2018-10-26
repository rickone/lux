#pragma once

#include "tcp_socket.h"
#include "resp_object.h"

namespace lux {

class RedisSocket : public TcpSocket
{
public:
    RedisSocket() = default;
    virtual ~RedisSocket() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<RedisSocket> connect(const char *node, const char *service);

    void on_recv_buffer(Buffer *buffer);
    int lua_request(lua_State *L);
    int lua_response(lua_State *L);

    template<typename...A>
    void request(A...args)
    {
        RespObject req(RespType::RESP_ARRAY);
        req.set_array(args...);

        std::string str;
        req.serialize(str);

        send(str.data(), str.size(), 0);
    }

    const RespObject &response() const { return _resp_response; }

    def_lua_callback(on_respond, RedisSocket *)

private:
    RespObject _resp_response;
};

} // lux
