#if !defined(_WIN32)
#include "unix_socket_stream.h"
#include "socket_manager.h"
#include "lux_core.h"

using namespace lux;

void UnixSocketStream::new_class(lua_State *L)
{
    lua_new_class(L, UnixSocketStream);

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "unix_connect", connect);
    }
    lua_pop(L, 1);

    lua_lib(L, "lux_core");
    {
        lua_std_method(L, fork);
    }
    lua_pop(L, 1);
}

std::pair<Socket, Socket> UnixSocketStream::create_pair()
{
    int fd[2] = { -1, -1 };
#ifdef __linux__
    int ret = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd);
#else
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
#endif
    if (ret != 0)
        throw_unix_error("socketpair");

    auto pair = std::make_pair(Socket(fd[0]), Socket(fd[1]));
#if !defined(__linux__)
    pair.first.set_nonblock();
    pair.second.set_nonblock();
#endif
    return pair;
}

std::shared_ptr<UnixSocketStream> UnixSocketStream::create(int fd)
{
    auto socket = ObjectManager::inst()->create<UnixSocketStream>();
    socket->attach(fd);
    socket->add_event(kSocketEvent_Read);
    return socket;
}

std::shared_ptr<UnixSocketStream> UnixSocketStream::connect(const char *socket_path)
{
    auto socket = ObjectManager::inst()->create<UnixSocketStream>();
    socket->init_connect(socket_path);
    return socket;
}

std::shared_ptr<UnixSocketStream> UnixSocketStream::fork(const char *proc_title, std::function<void (const std::shared_ptr<UnixSocketStream> &socket)> worker_proc)
{
    auto pair = create_pair();

    int ret = ::fork();
    if (ret < 0)
        throw_unix_error("fork");

    if (ret > 0)
    {
        log_info("socket fork fd(%d)", pair.first.fd());

        return create(pair.first.detach());
    }

    Core::inst()->set_proc_title(proc_title);

    int pid = getpid();
    Core::inst()->on_fork(pid);

    log_info("fork into child-process(%s) pid(%d) socket fd(%d)", proc_title, pid, pair.second.fd());

    pair.first.close();

    auto socket = create(pair.second.detach());
    worker_proc(socket);

    Core::inst()->run();
    _exit(0);
}

int UnixSocketStream::lua_fork(lua_State *L)
{
    const char *proc_title = luaL_checkstring(L, 1);

    if (lua_isstring(L, 2))
    {
        const char *file_path = lua_tostring(L, 2);

        int ret = luaL_loadfile(L, file_path);
        if (ret != LUA_OK)
            luaL_error(L, "loadfile(%s) error: %s", file_path, lua_tostring(L, -1));

        lua_remove(L, 2);
    }
    luaL_checktype(L, 2, LUA_TFUNCTION);

    auto socket_out = fork(proc_title, [L](const std::shared_ptr<UnixSocketStream> &socket_in){
        lua_remove(L, 1);
        lua_push(L, socket_in.get());
        lua_call(L, 1, 0);
    });
    lua_push(L, socket_out);
    return 1;
}

void UnixSocketStream::init_connect(const char *socket_path)
{
    logic_assert(_fd < 0, "_fd = %d", _fd);

    init(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;

    Socket::connect((const struct sockaddr *)&addr, sizeof(addr));
    add_event(kSocketEvent_Read);
}

int UnixSocketStream::send(const char *data, size_t len, int flags)
{
    if (!_send_buffer.empty())
    {
        _send_buffer.push(data, len);
        return 0;
    }

    while (len > 0)
    {
        int ret = sendto(data, len, 0, nullptr, 0);
        if (ret < 0)
            break;

        data += ret;
        len -= ret;
    }

    if (len > 0)
    {
        set_event(kSocketEvent_ReadWrite);
        _send_buffer.push(data, len);

        log_info("fd(%d) write pending", _fd);
    }
    return 0;
}

void UnixSocketStream::on_read(size_t len)
{
    while (_fd >= 0)
    {
        auto back = _recv_buffer.back();
        int ret = recvfrom(back.first, back.second, 0, nullptr, nullptr);
        if (ret == 0)
        {
            on_close(this);
            close();
            return;
        }

        if (ret < 0)
            break;

        _recv_buffer.push(nullptr, ret);

        on_recv(this, &_recv_buffer);
    }
}

void UnixSocketStream::on_write(size_t len)
{
    flush();

    if (_send_buffer.empty())
    {
        set_event(kSocketEvent_Read);

        log_debug("fd(%d) write reset", _fd);
    }
}

void UnixSocketStream::flush()
{
    while (!_send_buffer.empty())
    {
        auto front = _send_buffer.front();
        int ret = sendto(front.first, front.second, 0, nullptr, 0);
        if (ret < 0)
            break;

        _send_buffer.pop(nullptr, ret);
    }
}

#endif // !_WIN32
