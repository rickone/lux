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
        lua_method(L, connect);
        lua_method(L, push_fd);
        lua_method(L, pop_fd);
    }
    lua_setfield(L, -2, "__method");

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "unix_attach", create);
        lua_set_method(L, "unix_bind", bind);
    }
    lua_pop(L, 1);
}

std::shared_ptr<UnixSocket> UnixSocket::create(int fd)
{
    std::shared_ptr<UnixSocket> socket(new UnixSocket());
    socket->attach(fd);
    socket->add_event(kSocketEvent_Read);
    socket->_on_read = &UnixSocket::on_recvfrom;

    return socket;
}

std::shared_ptr<UnixSocket> UnixSocket::bind(const char *socket_path)
{
    std::shared_ptr<UnixSocket> socket(new UnixSocket());
    socket->init_bind(socket_path);
    return socket;
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
    add_event(kSocketEvent_Read);
    _socket_path = addr.sun_path;
    _on_read = &UnixSocket::on_recvfrom;
}

void UnixSocket::connect(const char *socket_path)
{
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;

    Socket::connect((const struct sockaddr *)&addr, sizeof(addr));
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
    (this->*_on_read)(len);
}

void UnixSocket::on_recvfrom(size_t len)
{
    while (_fd >= 0)
    {
        sockaddr_un remote_sockaddr;
        socklen_t remote_sockaddr_len;

        _recv_buffer.clear();
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
