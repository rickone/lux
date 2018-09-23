#include "socket_package.h"

void SocketPackage::new_class(lua_State *L)
{
    lua_new_class(L, SocketPackage);

    lua_newtable(L);
    {
        lua_std_method(L, recv);
        lua_std_method(L, send);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_callback(L, on_recv);
        lua_callback(L, on_send);
    }
    lua_setfield(L, -2, "__property");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_package", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<SocketPackage> SocketPackage::create()
{
    return std::make_shared<SocketPackage>();
}

void SocketPackage::recv(Buffer *buffer)
{
    while (!buffer->empty())
    {
        if (_package_len == 0)
        {
            if (buffer->size() < sizeof(_package_len))
                break;

            buffer->pop((char *)&_package_len, sizeof(_package_len));
        }

        if (buffer->size() < _package_len)
            break;

        _package_buffer.clear();
        buffer->pop_buffer(&_package_buffer, _package_len);
        on_recv(&_package_buffer);
        _package_len = 0;
    }
}

void SocketPackage::send(const char *data, size_t len)
{
    runtime_assert(len <= USHRT_MAX, "package too long to send. len = %u", len);

    uint16_t package_len = (uint16_t)len;
    RawBuffer rb[2];
    rb[0].data = (const char *)&package_len;
    rb[0].len = sizeof(package_len);
    rb[1].data = data;
    rb[1].len = len;

    on_send(rb, 2);
}

int SocketPackage::lua_recv(lua_State *L)
{
    Buffer *buffer = lua_to_ptr<Buffer>(L, 1);
    recv(buffer);
    return 0;
}

int SocketPackage::lua_send(lua_State *L)
{
    size_t len = 0;
    const char *data = luaL_checklstring(L, 1, &len);
    send(data, len);
    return 0;
}

