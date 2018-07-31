#include "udp_socket.h"

UdpSocket::UdpSocket(socket_t fd) : Socket(fd), _recv_buffer(), _on_read()
{
}

void UdpSocket::new_class(lua_State *L)
{
    lua_new_class(L, UdpSocket);

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "udp_bind", bind);
        lua_set_method(L, "udp_connect", connect);
    }
    lua_pop(L, 1);
}

std::shared_ptr<UdpSocket> UdpSocket::create(socket_t fd)
{
    std::shared_ptr<UdpSocket> socket(new UdpSocket());
    socket->attach(fd);
    socket->add_event(kSocketEvent_Read);
    socket->_on_read = &UdpSocket::on_recv;

#ifdef _WIN32
    socket->on_read(0);
#endif
    return socket;
}

std::shared_ptr<UdpSocket> UdpSocket::bind(const char *node, const char *service)
{
    std::shared_ptr<UdpSocket> socket(new UdpSocket());
    socket->init_bind(node, service);
    return socket;
}

std::shared_ptr<UdpSocket> UdpSocket::connect(const char *node, const char *service)
{
    std::shared_ptr<UdpSocket> socket(new UdpSocket());
    socket->init_connect(node, service);
    return socket;
}

void UdpSocket::init_bind(const char *node, const char *service)
{
    logic_assert(_fd == INVALID_SOCKET, "_fd = %d", _fd);

    any_addrinfo(node, service, SOCK_DGRAM, 0, [this](const struct addrinfo *ai){
        init(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        setsockopt(SOL_SOCKET, SO_REUSEADDR, true);
#if !defined(_WIN32)
        setsockopt(SOL_SOCKET, SO_REUSEPORT, true);
#endif
        Socket::bind(ai->ai_addr, (socklen_t)ai->ai_addrlen);
        add_event(kSocketEvent_Read);
        _on_read = &UdpSocket::on_recvfrom;

#ifdef _WIN32
        on_read(0);
#endif
    });
}

void UdpSocket::init_connect(const char *node, const char *service)
{
    logic_assert(_fd == INVALID_SOCKET, "_fd = %d", _fd);

    any_addrinfo(node, service, SOCK_DGRAM, 0, [this](const struct addrinfo *ai){
        init(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        Socket::connect(ai->ai_addr, (socklen_t)ai->ai_addrlen);
        add_event(kSocketEvent_Read);
        _on_read = &UdpSocket::on_recv;

#ifdef _WIN32
        on_read(0);
#endif
    });
}

void UdpSocket::on_recv_buffer(Buffer *buffer)
{
    publish(kMsg_SocketRecv, buffer);
}

void UdpSocket::on_read(size_t len)
{
    (this->*_on_read)(len);
}

void UdpSocket::on_recvfrom(size_t len)
{
#ifdef _WIN32
    if (len > 0)
    {
        _recv_buffer.push(nullptr, len);

        LuaSockAddr lua_sockaddr;
        lua_sockaddr.addr = (struct sockaddr *)&_remote_sockaddr;
        lua_sockaddr.addrlen = _remote_sockaddr_len;
        publish(kMsg_SocketRecv, &_recv_buffer, &lua_sockaddr);
    }
#endif

    while (_fd != INVALID_SOCKET)
    {
        _recv_buffer.clear();
        auto back = _recv_buffer.back();

        _remote_sockaddr_len = sizeof(_remote_sockaddr);
#ifdef _WIN32
        int ret = wsa_recvfrom(back.first, back.second, 0, (struct sockaddr *)&_remote_sockaddr, &_remote_sockaddr_len);
#else
        int ret = recvfrom(back.first, back.second, 0, (struct sockaddr *)&_remote_sockaddr, &_remote_sockaddr_len);
#endif
        if (ret < 0)
            break;

        _recv_buffer.push(nullptr, ret);

        LuaSockAddr lua_sockaddr;
        lua_sockaddr.addr = (struct sockaddr *)&_remote_sockaddr;
        lua_sockaddr.addrlen = _remote_sockaddr_len;
        publish(kMsg_SocketRecv, &_recv_buffer, &lua_sockaddr);
    }
}

void UdpSocket::on_recv(size_t len)
{
#ifdef _WIN32
    if (len > 0)
    {
        _recv_buffer.push(nullptr, len);

        publish(kMsg_SocketRecv, &_recv_buffer);
    }
#endif

    while (_fd != INVALID_SOCKET)
    {
        _recv_buffer.clear();
        auto back = _recv_buffer.back();

#ifdef _WIN32
        _remote_sockaddr_len = sizeof(_remote_sockaddr);
        int ret = wsa_recvfrom(back.first, back.second, 0, (struct sockaddr *)&_remote_sockaddr, &_remote_sockaddr_len);
#else
        int ret = recv(back.first, back.second, 0);
#endif
        if (ret < 0)
            break;

        _recv_buffer.push(nullptr, ret);

        publish(kMsg_SocketRecv, &_recv_buffer);
    }
}
