#include "redis_socket.h"

using namespace lux;

void RedisSocket::new_class(lua_State *L)
{
    lua_new_class(L, RedisSocket);

    lua_newtable(L);
    {
        lua_std_method(L, request);
        lua_std_method(L, response);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_callback(L, on_respond);
    }
    lua_setfield(L, -2, "__property");

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "redis_connect", connect);
    }
    lua_pop(L, 1);
}

std::shared_ptr<RedisSocket> RedisSocket::connect(const char *node, const char *service)
{
    auto socket = ObjectManager::inst()->create<RedisSocket>();
    socket->init_connection(node, service);

    std::function<void (RedisSocket *, Socket *, Buffer *)> func = [](RedisSocket *redis_socket, Socket *socket, Buffer *buffer) {
        redis_socket->on_recv_buffer(buffer);
    };
    socket->on_recv.set(socket.get(), func);
    return socket;
}

void RedisSocket::on_recv_buffer(Buffer *buffer)
{
    for (;;)
    {
        int crlf = buffer->find(0, "\r\n");
        if (crlf < 0)
            break;

        std::string line = buffer->pop_string((size_t)crlf + 2);
        if (_resp_response.parse_line(line))
        {
            on_respond(this);
            _resp_response.clear();
        }
    }
}

int RedisSocket::lua_request(lua_State *L)
{
    RespObject req(RespType::RESP_ARRAY);
    int top = lua_gettop(L);
    for (int i = 1; i <= top; ++i)
    {
        size_t len = 0;
        const char *data = lua_tolstring(L, i, &len);

        if (data == nullptr)
            return luaL_argerror(L, i, "cannot convert to string");

        RespObject obj(RespType::RESP_BULK_STRING);
        obj.set_value(data, len);

        req.push(obj);
    }

    std::string str;
    req.serialize(str);

    send(str.data(), str.size(), 0);
    return 0;
}

static int lua_push_resp_object(lua_State *L, const RespObject &obj)
{
    switch (obj.type())
    {
        case RespType::RESP_NULL:
        {
            lua_pushnil(L);
            break;
        }

        case RespType::RESP_SIMPLE_STRING:
        case RespType::RESP_ERROR:
        case RespType::RESP_BULK_STRING:
        {
            lua_pushlstring(L, obj.value().data(), obj.value().size());
            break;
        }

        case RespType::RESP_INTEGER:
        {
            lua_pushinteger(L, (lua_Integer)std::stol(obj.value()));
            break;
        }

        case RespType::RESP_ARRAY:
        {
            lua_createtable(L, (int)obj.array_size(), 0);

            lua_Integer i = 0;
            for (auto &member : obj)
            {
                lua_push_resp_object(L, member);
                lua_seti(L, -2, ++i);
            }
            break;
        }

        default:
            return luaL_error(L, "Unknown RESP object type: %d", (int)obj.type());
    }

    return 1;
}

int RedisSocket::lua_response(lua_State *L)
{
    return lua_push_resp_object(L, _resp_response);
}
