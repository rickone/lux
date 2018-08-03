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
    _stream_mode = stream_mode;
}

void Messenger::on_recv_stream(LuaObject *msg_object)
{
    Buffer *buffer = (Buffer *)msg_object;

    if (_package_len == 0)
    {
        if (buffer->size() < sizeof(_package_len))
            return;

        buffer->get(0, (char *)&_package_len, sizeof(_package_len));
        runtime_assert(_package_len > sizeof(_package_len), "package_len(%u)", _package_len);
    }

    if (buffer->size() < _package_len)
        return;

    buffer->pop(nullptr, sizeof(_package_len));
    std::string str = buffer->pop_string(_package_len - sizeof(_package_len));
    on_recv_package(str);

    _package_len = 0;
}

void Messenger::on_recv_dgram(LuaObject *msg_object)
{
    Buffer *buffer = (Buffer *)msg_object;

    std::string str = buffer->pop_string(buffer->size());
    on_recv_package(str);
}

void Messenger::on_recv_package(const std::string &str)
{
    lua_State *L = lua_state;
    int top = lua_gettop(L);
    int n = lua_proto_unpack_args(L, str);

    LuaMessageObject msg_object;
    msg_object.arg_begin = top + 1;
    msg_object.arg_end = top + n;

    publish_msg(kMsg_RemoteCall, &msg_object);
    lua_settop(L, top);
}

int Messenger::lua_send(lua_State *L)
{
    int top = lua_gettop(L);

    std::string str;
    if (_stream_mode)
    {
        str.append("  ", 2);
        lua_proto_pack_args(str, L, top);

        runtime_assert(str.size() <= USHRT_MAX, "pack_len(%u)", str.size());

        uint16_t package_len = (uint16_t)str.size();
        str.replace(0, 2, (const char *)&package_len, 2);
    }
    else
    {
        lua_proto_pack_args(str, L, top);
    }

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
