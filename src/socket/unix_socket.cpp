#if !defined(_WIN32)
#include "unix_socket.h"
#include "socket_addr.h"
#include "config.h"
#include "error.h"
#include "system_manager.h"

UnixSocket::~UnixSocket()
{
    if (!_socket_path.empty())
    {
        unlink(_socket_path.c_str());
        _socket_path.clear();
    }
}

void UnixSocket::new_class(lua_State *L)
{
    lua_new_class(L, UnixSocket);

    lua_newtable(L);
    {
        lua_method(L, push_fd);
        lua_method(L, pop_fd);
    }
    lua_setfield(L, -2, "__method");

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "unix_attach", create);
        lua_set_method(L, "unix_connect", connect);
    }
    lua_pop(L, 1);

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "fork", lua_fork);
    }
    lua_pop(L, 1);
}

std::pair<Socket, Socket> UnixSocket::create_pair()
{
    int fd[2] = { -1, -1 };
#ifdef __linux__
    int ret = socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, fd);
#else
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
#endif
    if (ret != 0)
        throw_system_error(errno, "socketpair");

    auto pair = std::make_pair(Socket(fd[0]), Socket(fd[1]));
#if !defined(__linux__)
    pair.first.set_nonblock();
    pair.second.set_nonblock();
#endif
    return pair;
}

std::shared_ptr<UnixSocket> UnixSocket::create(int fd, bool stream_mode)
{
    std::shared_ptr<UnixSocket> socket(new UnixSocket());
    socket->attach(fd);
    socket->add_event(kSocketEvent_Read);
    if (stream_mode)
        socket->_on_read = &UnixSocket::on_recv;
    else
        socket->_on_read = &UnixSocket::on_recvfrom;

    return socket;
}

std::shared_ptr<UnixSocket> UnixSocket::bind(const char *socket_path)
{
    std::shared_ptr<UnixSocket> socket(new UnixSocket());
    socket->init_bind(socket_path);
    return socket;
}

std::shared_ptr<UnixSocket> UnixSocket::connect(const char *socket_path, bool stream_mode)
{
    std::shared_ptr<UnixSocket> socket(new UnixSocket());
    socket->init_connect(socket_path, stream_mode);
    return socket;
}

std::shared_ptr<UnixSocket> UnixSocket::fork(const char *proc_title, std::function<void (const std::shared_ptr<UnixSocket> &socket)> worker_proc)
{
    auto pair = create_pair();

    int ret = ::fork();
    if (ret < 0)
        throw_system_error(errno, "fork");

    if (ret > 0)
    {
        log_info("socket fork fd(%d)", pair.first.fd());

        return create(pair.first.detach(), true);
    }

    system_manager->set_proc_title(proc_title);

    int pid = getpid();
    system_manager->on_fork(pid);

    log_info("fork into child-process(%s) pid(%d) socket fd(%d)", proc_title, pid, pair.second.fd());

    pair.first.close();

    auto socket = create(pair.second.detach(), true);

    worker_proc(socket);

    system_manager->run();

    delete system_manager;
    _exit(0);
}

int UnixSocket::lua_fork(lua_State *L)
{
    const char *proc_title = luaL_checkstring(L, 1);
    luaL_argcheck(L, lua_isfunction(L, 2), 2, "need a function");

    auto socket_out = fork(proc_title, [L](const std::shared_ptr<UnixSocket> &socket_in){
        lua_push(L, socket_in);
        int ret = lua_btcall(L, 1, 0);
        if (ret != LUA_OK)
            throw_lua_error(L);
    });
    lua_push(L, socket_out);
    return 1;
}

void UnixSocket::init_bind(const char *socket_path)
{
    logic_assert(_fd < 0, "_fd = %d", _fd);

    init(AF_UNIX, SOCK_DGRAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;

    unlink(addr.sun_path);

    Socket::bind((const struct sockaddr *)&addr, sizeof(addr));
    listen(config->env()->listen_backlog);
    add_event(kSocketEvent_Read);
    _socket_path = addr.sun_path;
    _on_read = &UnixSocket::on_recvfrom;
}

void UnixSocket::init_connect(const char *socket_path, bool stream_mode)
{
    logic_assert(_fd < 0, "_fd = %d", _fd);

    if (stream_mode)
        init(AF_UNIX, SOCK_STREAM, 0);
    else
        init(AF_UNIX, SOCK_DGRAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;

    Socket::connect((const struct sockaddr *)&addr, sizeof(addr));
    add_event(kSocketEvent_Read);

    if (stream_mode)
        _on_read = &UnixSocket::on_recv;
    else
        _on_read = &UnixSocket::on_recvfrom;
}

void UnixSocket::push_fd(int fd)
{
    _send_fd_list.push_back(fd);
}

int UnixSocket::pop_fd()
{
    if (_recv_fd_list.empty())
        return -1;

    int fd = _recv_fd_list.front();
    _recv_fd_list.pop_front();
    return fd;
}

int UnixSocket::recvfrom(char *data, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    struct msghdr msg;
    struct iovec iov;
    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;

    iov.iov_base = data;
    iov.iov_len = len;

    msg.msg_name = src_addr;
    msg.msg_namelen = addrlen ? *addrlen : 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);

    int ret = recvmsg(_fd, &msg, flags);
    if (ret < 0)
    {
        if (errno == EAGAIN)
            return -1;

        throw_socket_error();
    }

    if (addrlen)
        *addrlen = msg.msg_namelen;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
    {
        int fd = *((int *)CMSG_DATA(cmsg));
        log_debug("fd(%d) recv %d bytes with fd(%d)", _fd, ret, fd);

        Socket socket(fd);
        socket.set_nonblock();

        _recv_fd_list.push_back(socket.detach());
    }
    else
    {
        log_debug("fd(%d) recv %d bytes", _fd, ret);
    }
    return ret;
}

int UnixSocket::sendto(const char *data, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    struct msghdr msg;
    struct iovec iov;

    iov.iov_base = const_cast<char *>(data);
    iov.iov_len = len;

    msg.msg_name = const_cast<struct sockaddr *>(dest_addr);
    msg.msg_namelen = addrlen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = nullptr;
    msg.msg_controllen = 0;

    int fd = -1;
    if (!_send_fd_list.empty())
        fd = _send_fd_list.front();

    if (fd >= 0)
    {
        iov.iov_len = 1;

        union {
            struct cmsghdr cmsghdr;
            char control[CMSG_SPACE(sizeof(int))];
        } cmsgu;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        *((int *)CMSG_DATA(cmsg)) = fd;
    }

    int ret = sendmsg(_fd, &msg, flags);
    if (ret < 0)
    {
        if (errno == EAGAIN)
            return -1;

        throw_socket_error();
    }

    if (fd >= 0)
    {
        _send_fd_list.pop_front();
        log_debug("fd(%d) send %d bytes with fd(%d)", _fd, ret, fd);

        ::close(fd);
    }
    else
    {
        log_debug("fd(%d) send %d bytes", _fd, ret);
    }
    return ret;
}

int UnixSocket::send(const char *data, size_t len, int flags)
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

void UnixSocket::on_read(size_t len)
{
    while (_fd >= 0)
    {
        auto back = _recv_buffer.back();
        int ret = recv(back.first, back.second, 0);
        if (ret == 0)
        {
            publish(kMsg_SocketClose, (Socket *)this);

            close();
            return;
        }

        if (ret < 0)
            break;

        _recv_buffer.push(nullptr, ret);

        publish(kMsg_SocketRecv, &_recv_buffer);
    }
}

void UnixSocket::on_write(size_t len)
{
    flush();

    if (_send_buffer.empty())
    {
        set_event(kSocketEvent_Read);

        log_info("fd(%d) write reset", _fd);
    }
}

void UnixSocket::flush()
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

void UnixSocket::on_recvfrom(size_t len)
{
    while (_fd >= 0)
    {
        sockaddr_storage remote_sockaddr;
        socklen_t remote_sockaddr_len;

        auto back = _recv_buffer.back();
        int ret = recvfrom(back.first, back.second, 0, (struct sockaddr *)&remote_sockaddr, &remote_sockaddr_len);
        if (ret == 0)
        {
            publish(kMsg_SocketClose, (Socket *)this);

            close();
            return;
        }

        if (ret < 0)
            break;

        _recv_buffer.push(nullptr, ret);

        LuaSockAddr lua_sockaddr;
        lua_sockaddr.addr = (struct sockaddr *)&remote_sockaddr;
        lua_sockaddr.addrlen = remote_sockaddr_len;

        publish(kMsg_SocketRecv, &_recv_buffer, &lua_sockaddr);
    }
}

void UnixSocket::on_recv(size_t len)
{
    while (_fd >= 0)
    {
        auto back = _recv_buffer.back();
        int ret = recvfrom(back.first, back.second, 0, nullptr, nullptr);
        if (ret == 0)
        {
            publish(kMsg_SocketClose, (Socket *)this);

            close();
            return;
        }

        if (ret < 0)
            break;

        _recv_buffer.push(nullptr, ret);

        publish(kMsg_SocketRecv, &_recv_buffer);
    }
}

#endif // !_WIN32
