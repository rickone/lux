#include "socket_msg.h"
#include "entity.h"

void SocketPackage::new_class(lua_State *L)
{
    lua_new_class(L, SocketPackage);

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

std::shared_ptr<SocketPackage> SocketPackage::create(int msg_type)
{
    std::shared_ptr<SocketPackage> msgr(new SocketPackage());
    msgr->init(msg_type);
    return msgr;
}

void SocketPackage::init(int msg_type)
{
    subscribe(kMsg_SocketRecv, &SocketPackage::on_recv);
}

void SocketPackage::on_recv(LuaObject *msg_object)
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

    LuaPackage package;
    package.str = str;
    publish(kMsg_PackageRecv, &package);

    _package_len = 0;
}

int SocketPackage::lua_pack(lua_State *L)
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
