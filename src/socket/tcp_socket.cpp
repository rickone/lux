#include "tcp_socket.h"
#include "socket_manager.h"
#include "config.h"

TcpSocket::TcpSocket() : Socket(), _connected(), _recv_buffer(Config::env()->socket_recv_buffer_init), _send_buffer(Config::env()->socket_send_buffer_init)
{
}

void TcpSocket::new_class(lua_State *L)
{
    lua_new_class(L, TcpSocket);

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "tcp_connect", connect);
    }
    lua_pop(L, 1);
}

std::shared_ptr<TcpSocket> TcpSocket::create(socket_t fd)
{
    auto socket = SocketManager::inst()->create<TcpSocket>();
    if (fd != INVALID_SOCKET)
    {
        socket->attach(fd);
        socket->setsockopt(IPPROTO_TCP, TCP_NODELAY, true);
        socket->add_event(kSocketEvent_Read);
        socket->_connected = true;

#ifdef _WIN32
        socket->on_read(0);
#endif
    }
    return socket;
}

std::shared_ptr<TcpSocket> TcpSocket::connect(const char *node, const char *service)
{
    auto socket = SocketManager::inst()->create<TcpSocket>();
    socket->init_connection(node, service);
    return socket;
}

void TcpSocket::init_connection(const char *node, const char *service)
{
    logic_assert(_fd == INVALID_SOCKET, "_fd = %d", _fd);

    any_addrinfo(node, service, SOCK_STREAM, 0, [this](const addrinfo *ai){
        init(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        setsockopt(IPPROTO_TCP, TCP_NODELAY, true);
        add_event(kSocketEvent_ReadWrite);
#ifdef _WIN32
        BOOL succ = connect_ex(ai->ai_addr, (socklen_t)ai->ai_addrlen);
        _connected = (succ != 0);

        if (_connected)
            on_read(0);
#else
        _connected = Socket::connect(ai->ai_addr, ai->ai_addrlen);
#endif
    });
}

void TcpSocket::send_pending(const char *data, size_t len)
{
    _send_buffer.push(data, len);
    if (_send_buffer.size() > Config::env()->socket_send_buffer_max)
    {
        socket_t fd = _fd;
        on_error(this, -1);
        throw_error(std::runtime_error, "fd(%d) send_pending buffer(%u Byte) overflow", fd, _send_buffer.size());
    }

    log_info("fd(%d) write pending", _fd);
}

int TcpSocket::send(const char *data, size_t len, int flags)
{
    runtime_assert(_connected, "send error: fd(%d) not connected", _fd);

    if (!_send_buffer.empty())
    {
        send_pending(data, len);
        return 0;
    }

    while (len > 0)
    {
#ifdef _WIN32
        int ret = Socket::send(data, len, flags);
#else
        int ret = write(data, len);
#endif
        if (ret < 0)
        {
#ifdef _WIN32
            wsa_send(nullptr, 0, 0);
#endif
            break;
        }

        data += ret;
        len -= ret;
    }

    if (len > 0)
    {
        set_event(kSocketEvent_ReadWrite);
        send_pending(data, len);
    }
    return 0;
}

void TcpSocket::on_read(size_t len)
{
    while (_fd != INVALID_SOCKET)
    {
        auto back = _recv_buffer.back();
#ifdef _WIN32
        int ret = recv(back.first, back.second, 0);
#else
        int ret = read(back.first, back.second);
#endif
        if (ret == 0)
        {
            on_close(this);
            close();
            return;
        }

        if (ret < 0)
        {
#ifdef _WIN32
            wsa_recv(nullptr, 0, 0);
#endif
            break;
        }

        _recv_buffer.push(nullptr, ret);

        on_recv(this, &_recv_buffer);
    }
}

void TcpSocket::on_write(size_t len)
{
    if (!_connected)
    {
#ifdef _WIN32
        int connect_time = 0;
        getsockopt(SOL_SOCKET, SO_CONNECT_TIME, &connect_time);

        if (connect_time == -1)
        {
            on_error(this, -2);
            throw_error(std::runtime_error, "ConnectEx error: SO_CONNECT_TIME = %d", connect_time);
        }
#endif

        log_info("fd(%d) connected", _fd);
        _connected = true;

        on_connect(this);

#ifdef _WIN32
        on_read(0);
#endif
    }

    flush();

    if (_send_buffer.empty())
    {
        set_event(kSocketEvent_Read);

        log_debug("fd(%d) write reset", _fd);
    }
}

#ifdef _WIN32
std::shared_ptr<TcpSocket> TcpSocket::get_accept_ex_socket(LPWSAOVERLAPPED ovl)
{
    TcpSocket *socket = CONTAINING_RECORD(ovl, TcpSocket, _read_ovl);

    return std::static_pointer_cast<TcpSocket>(socket->shared_from_this());
}

BOOL TcpSocket::accept_ex(socket_t listen_fd, int family, int socktype, int protocol)
{
    init(family, socktype, protocol);

    BOOL succ = SocketManager::inst()->accept_ex(listen_fd, _fd, _accept_ex_buffer, ACCEPT_EX_ADDRESS_LEN, ACCEPT_EX_ADDRESS_LEN, &_read_ovl);
    if (!succ)
    {
        int err = get_socket_error();
        if (err != ERROR_IO_PENDING)
            throw_socket_error();

        //++_ovl_ref; not here but listen-socket
    }
    log_info("fd(%d) accept_ex fd(%d) ... %s", listen_fd, _fd, (succ ? "OK" : "pending"));

    if (succ)
        accept_ex_complete();
    return succ;
}

void TcpSocket::accept_ex_complete()
{
    struct sockaddr *local_sockaddr = nullptr;
    socklen_t local_sockaddr_len = 0;
    struct sockaddr *remote_sockaddr = nullptr;
    socklen_t remote_sockaddr_len = 0;

    SocketManager::inst()->get_accept_ex_sockaddrs(_accept_ex_buffer, ACCEPT_EX_ADDRESS_LEN, ACCEPT_EX_ADDRESS_LEN,
        &local_sockaddr, &local_sockaddr_len, &remote_sockaddr, &remote_sockaddr_len);

    auto name = get_addrname(remote_sockaddr, remote_sockaddr_len);
    log_info("fd(%d) is complete from %s", _fd, name.c_str());

    setsockopt(IPPROTO_TCP, TCP_NODELAY, true);
    add_event(kSocketEvent_Read);
    _connected = true;

    on_read(0);
}

BOOL TcpSocket::connect_ex(const struct sockaddr *addr, socklen_t addrlen)
{
    bind_any(addr->sa_family);

    BOOL connected = SocketManager::inst()->connect_ex(_fd, addr, addrlen, &_write_ovl);
    if (!connected)
    {
        int err = get_socket_error();
        if (err != ERROR_IO_PENDING)
            throw_socket_error();

        ++_ovl_ref;
    }

    auto name = get_addrname(addr, addrlen);
    log_info("fd(%d) connect_ex %s ... %s", _fd, name.c_str(), (connected ? "OK" : "pending"));
    return connected;
}
#endif // _WIN32

void TcpSocket::flush()
{
    while (!_send_buffer.empty())
    {
        auto front = _send_buffer.front();
#ifdef _WIN32
        int ret = Socket::send(front.first, front.second, 0);
#else
        int ret = write(front.first, front.second);
#endif
        if (ret < 0)
        {
#ifdef _WIN32
            wsa_send(nullptr, 0, 0);
#endif
            break;
        }

        _send_buffer.pop(nullptr, ret);
    }
}
