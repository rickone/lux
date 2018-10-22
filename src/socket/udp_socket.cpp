#include "udp_socket.h"
#include "socket_manager.h"

using namespace lux;

UdpSocket::UdpSocket(socket_t fd) : Socket(fd)
{
}

void UdpSocket::new_class(lua_State *L)
{
    lua_new_class(L, UdpSocket);

    lua_newtable(L);
    {
        lua_method(L, set_reliable);
    }
    lua_setfield(L, -2, "__method");

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "udp_bind", bind);
        lua_set_method(L, "udp_connect", connect);
    }
    lua_pop(L, 1);
}

std::shared_ptr<UdpSocket> UdpSocket::create(socket_t fd)
{
    auto socket = ObjectManager::inst()->create<UdpSocket>();
    socket->attach(fd);
    socket->add_event(kSocketEvent_Read);
    socket->_on_read = &UdpSocket::do_recv;

#ifdef _WIN32
    socket->on_read(0);
#endif
    return socket;
}

std::shared_ptr<UdpSocket> UdpSocket::bind(const char *node, const char *service)
{
    auto socket = ObjectManager::inst()->create<UdpSocket>();
    socket->init_bind(node, service);
    return socket;
}

std::shared_ptr<UdpSocket> UdpSocket::connect(const char *node, const char *service)
{
    auto socket = ObjectManager::inst()->create<UdpSocket>();
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
        _on_read = &UdpSocket::do_recvfrom;

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
        _on_read = &UdpSocket::do_recv;

#ifdef _WIN32
        on_read(0);
#endif
    });
}

void UdpSocket::on_recv_buffer(Buffer *buffer)
{
    if (_kcp)
    {
        _kcp->recv(buffer->data(), buffer->size());
        return;
    }

    on_recv(this, buffer);
}

void UdpSocket::send_rawdata(RawBuffer *rb)
{
    Socket::send(rb->data, rb->len, 0);
}

void UdpSocket::set_reliable()
{
    _kcp = SocketKcp::create();

    std::function<void (UdpSocket *, Buffer *)> kcp_recv_wrap = [](UdpSocket *socket, Buffer *buffer){
        socket->on_recv(socket, buffer);
    };
    _kcp->on_recv.set(this, kcp_recv_wrap);

    std::function<void (UdpSocket *, RawBuffer *)> kcp_send_wrap = std::mem_fn(&UdpSocket::send_rawdata);
    _kcp->on_send.set(this, kcp_send_wrap);
}

int UdpSocket::send(const char *data, size_t len, int flags)
{
    if (_kcp)
    {
        _kcp->send(data, len);
        return (int)len;
    }

    return Socket::send(data, len, flags);
}

void UdpSocket::on_read(size_t len)
{
    (this->*_on_read)(len);
}

void UdpSocket::do_recvfrom(size_t len)
{
#ifdef _WIN32
    if (len > 0)
    {
#ifdef _DEBUG
        auto name = get_addrname((struct sockaddr *)&_remote_sockaddr, _remote_sockaddr_len);
        log_debug("fd(%d) wsa-recv %u bytes from %s", _fd, len, name.c_str());
#endif
        _recv_buffer.push(nullptr, len);

        RawBuffer rb;
        rb.data = (char *)&_remote_sockaddr;
        rb.len = (size_t)_remote_sockaddr_len;
        on_recvfrom(this, &_recv_buffer, &rb);
    }
#endif

    while (_fd != INVALID_SOCKET)
    {
        _recv_buffer.clear();
        auto back = _recv_buffer.back();

        _remote_sockaddr_len = sizeof(_remote_sockaddr);
        int ret = recvfrom(back.first, back.second, 0, (struct sockaddr *)&_remote_sockaddr, &_remote_sockaddr_len);
        if (ret < 0)
        {
#ifdef _WIN32
            wsa_recvfrom(back.first, back.second, 0, (struct sockaddr *)&_remote_sockaddr, &_remote_sockaddr_len);
#endif
            break;
        }

        _recv_buffer.push(nullptr, ret);

        RawBuffer rb;
        rb.data = (char *)&_remote_sockaddr;
        rb.len = (size_t)_remote_sockaddr_len;
        on_recvfrom(this, &_recv_buffer, &rb);
    }
}

void UdpSocket::do_recv(size_t len)
{
#ifdef _WIN32
    if (len > 0)
    {
#ifdef _DEBUG
        log_debug("fd(%d) wsa-recv %u bytes", _fd, len);
#endif
        _recv_buffer.push(nullptr, len);

        on_recv_buffer(&_recv_buffer);
    }
#endif

    while (_fd != INVALID_SOCKET)
    {
        _recv_buffer.clear();
        auto back = _recv_buffer.back();

        int ret = recv(back.first, back.second, 0);
        if (ret < 0)
        {
#ifdef _WIN32
            wsa_recv(back.first, back.second, 0);
#endif
            break;
        }

        _recv_buffer.push(nullptr, ret);

        on_recv_buffer(&_recv_buffer);
    }
}
