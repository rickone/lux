#include "lux_msgr.h"
#include "entity.h"

void Messenger::new_class(lua_State *L)
{
    lua_new_class(L, Messenger);

    lua_newtable(L);
    {
        lua_std_method(L, send);
    }
    lua_setfield(L, -2, "__method");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_msgr", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<Messenger> Messenger::create(int msg_type, bool stream_mode)
{
    std::shared_ptr<Messenger> msgr(new Messenger());
    msgr->init(msg_type, stream_mode);
    return msgr;
}

void Messenger::init(int msg_type, bool stream_mode)
{
    if (stream_mode)
        subscribe(msg_type, &Messenger::on_recv_stream);
    else
        subscribe(msg_type, &Messenger::on_recv_dgram);
}

void Messenger::on_recv_stream(LuaObject *msg_object)
{
    Buffer *buffer = (Buffer *)msg_object;

    if (_header_len == 0)
    {
        if (buffer->size() < sizeof(_header_len))
            return;

        buffer->pop((char *)&_header_len, sizeof(_header_len));
    }

    if (buffer->size() < _header_len)
        return;

    on_recv_package(buffer, _header_len);
}

void Messenger::on_recv_dgram(LuaObject *msg_object)
{
    Buffer *buffer = (Buffer *)msg_object;

    on_recv_package(buffer, buffer->size());
}

void Messenger::on_recv_package(Buffer *buffer, size_t len)
{
    lua_State *L = lua_state;

    int top = lua_gettop(L);
    std::string str = buffer->pop_string(len);
    int n = lux_proto_unpack_args(L, str);

    LuaMessageObject msg_object;
    msg_object.arg_begin = top + 1;
    msg_object.arg_end = top + n;

    publish_msg(kMsg_RemoteCall, &msg_object);

    lua_settop(L, top);
}

int Messenger::lua_send(lua_State *L)
{
    int top = lua_gettop(L);

    std::string str("xx", 2);
    lux_proto_pack_args(str, L, top);

    runtime_assert(str.size() <= USHRT_MAX, "pack_len(%u)", str.size());

    uint16_t package_len = (uint16_t)str.size() - 2;
    str.replace(0, 2, (const char *)&package_len, 2);

    _socket->send(str.data(), str.size(), 0);
    return 0;
}

void Messenger::start(LuaObject *init_object)
{
    _socket = std::dynamic_pointer_cast<Socket>(_entity->find_component("socket"));
    runtime_assert(_socket, "Messenger require 'Socket'");
}

void Messenger::stop() noexcept
{

}
