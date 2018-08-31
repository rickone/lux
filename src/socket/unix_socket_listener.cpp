#if !defined(_WIN32)
#include "unix_socket_listener.h"
#include "unix_socket_stream.h"
#include "config.h"

UnixSocketListener::~UnixSocketListener()
{
    if (!_socket_path.empty())
    {
        unlink(_socket_path.c_str());
        _socket_path.clear();
    }
}

void UnixSocketListener::new_class(lua_State *L)
{
    lua_new_class(L, UnixSocketListener);

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "unix_listen", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<UnixSocketListener> UnixSocketListener::create(const char *socket_path)
{
    std::shared_ptr<UnixSocketListener> socket(new UnixSocketListener());
    socket->init_service(socket_path);
    return socket;
}

void UnixSocketListener::init_service(const char *socket_path)
{
    logic_assert(_fd < 0, "_fd = %d", _fd);

    init(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;

    unlink(addr.sun_path);

    bind((const struct sockaddr *)&addr, sizeof(addr));
    listen(Config::env()->listen_backlog);
    add_event(kSocketEvent_Read);
    _socket_path = addr.sun_path;
}

void UnixSocketListener::on_read(size_t len)
{
    for (;;)
    {
        auto socket = accept();
        if (!socket)
            break;

        auto unix_socket = UnixSocketStream::create(socket.detach());
        on_accept(this, unix_socket.get());
    }
}

#endif // !_WIN32
