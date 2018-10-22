#include "socket_package.h"

using namespace lux;

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
            if (_header == 0)
            {
                if (buffer->size() < sizeof(_header))
                    break;

                buffer->pop((char *)&_header, sizeof(_header));
            }

            if (_header & 0x8000)
            {
                uint16_t tail = 0;
                if (buffer->size() < sizeof(tail))
                    break;

                buffer->pop((char *)&tail, sizeof(tail));

                _package_len = ((size_t)(_header & 0x7fff) | ((size_t)tail << 15));
            }
            else
            {
                _package_len = (size_t)_header;
            }
        }

        if (buffer->size() < _package_len)
            break;

        _package_buffer.clear();
        buffer->pop_buffer(&_package_buffer, _package_len);
        on_recv(&_package_buffer);

        _header = 0;
        _package_len = 0;
    }
}

void SocketPackage::send(const char *data, size_t len)
{
    runtime_assert(len <= 0x7fff'ffff, "package too long to send. len = %u", len);

    RawBuffer rb[2];
    uint16_t header[2];

    rb[0].data = (const char *)&header[0];
    if (len <= 0x7fff)
    {
        header[0] = (uint16_t)len;
        rb[0].len = 2;
    }
    else
    {
        header[0] = (uint16_t)(len & 0x7fff) | 0x8000;
        header[1] = (uint16_t)(len >> 15);
        rb[0].len = 4;
    }

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

