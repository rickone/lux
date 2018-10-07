#if !defined(_WIN32)
#include "udp_socket_listener.h"
#include <cstring> // memcpy
#include "udp_socket.h"

UdpSocketListener::UdpSocketListener(socket_t fd) : Socket(fd), _local_sockinfo(), _recv_buffer()
{
}

void UdpSocketListener::new_class(lua_State *L)
{
    lua_new_class(L, UdpSocketListener);

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "udp_listen", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<UdpSocketListener> UdpSocketListener::create(const char *node, const char *service)
{
    std::shared_ptr<UdpSocketListener> socket(new UdpSocketListener());
    socket->init_service(node, service);
    return socket;
}

void UdpSocketListener::init_service(const char *node, const char *service)
{
    logic_assert(_fd == INVALID_SOCKET, "_fd = %d", _fd);

    any_addrinfo(node, service, SOCK_DGRAM, 0, [this](const struct addrinfo *ai){
        init(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        setsockopt(SOL_SOCKET, SO_REUSEADDR, true);
        setsockopt(SOL_SOCKET, SO_REUSEPORT, true);
        bind(ai->ai_addr, (socklen_t)ai->ai_addrlen);
        add_event(kSocketEvent_Read);

        _local_sockinfo.family = ai->ai_family;
        _local_sockinfo.socktype = ai->ai_socktype;
        _local_sockinfo.protocol = ai->ai_protocol;

        memcpy(&_local_sockaddr, ai->ai_addr, ai->ai_addrlen);
        _local_sockaddr_len = (socklen_t)ai->ai_addrlen;
    });
}

void UdpSocketListener::do_accept()
{
    std::string key = get_sockaddr_key((const struct sockaddr *)&_remote_sockaddr, _remote_sockaddr_len);
    auto it = _accepted_sockets.find(key);
    if (it != _accepted_sockets.end())
    {
        it->second->on_recv_buffer(&_recv_buffer);
        return;
    }

    Socket socket;
    socket.init(_local_sockinfo.family, _local_sockinfo.socktype, _local_sockinfo.protocol);
    socket.setsockopt(SOL_SOCKET, SO_REUSEADDR, true);
    socket.setsockopt(SOL_SOCKET, SO_REUSEPORT, true);
    socket.bind((const struct sockaddr *)&_local_sockaddr, _local_sockaddr_len);
    socket.connect((const struct sockaddr *)&_remote_sockaddr, _remote_sockaddr_len);

    auto udp_socket = UdpSocket::create(socket.detach());
    on_accept(this, udp_socket.get());

    udp_socket->on_recv_buffer(&_recv_buffer);
}

void UdpSocketListener::on_read(size_t len)
{
    _accepted_sockets.clear();

    while (_fd != INVALID_SOCKET)
    {
        _recv_buffer.clear();
        auto back = _recv_buffer.back();

        _remote_sockaddr_len = sizeof(_remote_sockaddr);
        int ret = recvfrom(back.first, back.second, 0, (struct sockaddr *)&_remote_sockaddr, &_remote_sockaddr_len);
        if (ret < 0)
            break;

        _recv_buffer.push(nullptr, ret);

        do_accept();
    }
}

std::string UdpSocketListener::get_sockaddr_key(const struct sockaddr *addr, socklen_t addrlen)
{
    std::string key;

    if (addr->sa_family == AF_INET)
    {
        const sockaddr_in *sockaddr = (const sockaddr_in *)addr;
        key.append((const char *)&sockaddr->sin_port, sizeof(sockaddr->sin_port));
        key.append((const char *)&sockaddr->sin_addr.s_addr, sizeof(sockaddr->sin_addr.s_addr));
    }
    else if (addr->sa_family == AF_INET6)
    {
        const sockaddr_in6 *sockaddr = (const sockaddr_in6 *)addr;
        key.append((const char *)&sockaddr->sin6_port, sizeof(sockaddr->sin6_port));
        key.append((const char *)&sockaddr->sin6_addr.s6_addr, sizeof(sockaddr->sin6_addr.s6_addr));
    }
    else
        throw_error(std::runtime_error, "Unknown sockaddr: sa_family = %u, addrlen = %u", addr->sa_family, addrlen);

    return key;
}

#endif // !_WIN32
