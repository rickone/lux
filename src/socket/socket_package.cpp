#include "socket_package.h"
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
        lua_set_method(L, "create_package", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<SocketPackage> SocketPackage::create()
{
    return std::shared_ptr<SocketPackage>(new SocketPackage());
}

void SocketPackage::on_socket_recv(Socket *socket, Buffer *buffer)
{
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
    _callback(this, &package);

    _package_len = 0;
}

int SocketPackage::lua_send(lua_State *L)
{
    size_t len = 0;
    const char *data = luaL_checklstring(L, 1, &len);

    if (len >= USHRT_MAX)
        return luaL_argerror(L, 1, "too long to send");

    uint16_t header = (uint16_t)len;

    _socket->send((const char *)&header, sizeof(header), 0);
    _socket->send(data, len, 0);
    return 0;
}

void SocketPackage::start()
{
    _socket = std::static_pointer_cast<Socket>(_entity->find_component("socket"));
    _socket->on_recv.set(this, &SocketPackage::on_socket_recv);
}
