#include "tcp_socket_listener.h"
#include "socket_manager.h"
#include "config.h"

void TcpSocketListener::new_class(lua_State *L)
{
    lua_new_class(L, TcpSocketListener);

    lua_lib(L, "socket_core");
    {
        lua_set_method(L, "tcp_listen", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<TcpSocketListener> TcpSocketListener::create(const char *node, const char *service)
{
    auto socket = SocketManager::inst()->create<TcpSocketListener>();
    socket->init_service(node, service);
    return socket;
}

void TcpSocketListener::init_service(const char *node, const char *service)
{
    logic_assert(_fd == INVALID_SOCKET, "_fd = %d", _fd);

    int backlog = Config::env()->listen_backlog;
    any_addrinfo(node, service, SOCK_STREAM, AI_PASSIVE, [this,backlog](const struct addrinfo *ai){
        init(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        setsockopt(SOL_SOCKET, SO_REUSEADDR, true);
#if !defined(_WIN32)
        setsockopt(SOL_SOCKET, SO_REUSEPORT, true);
#endif
        bind(ai->ai_addr, (socklen_t)ai->ai_addrlen);
        listen(backlog);
        add_event(kSocketEvent_Read);

#ifdef _WIN32
        _local_sockinfo.family = ai->ai_family;
        _local_sockinfo.socktype = ai->ai_socktype;
        _local_sockinfo.protocol = ai->ai_protocol;

        for (int i = 0; i < 8; ++i)
        {
            auto tcp_socket = TcpSocket::create();
            BOOL succ = tcp_socket->accept_ex(_fd, _local_sockinfo.family, _local_sockinfo.socktype, _local_sockinfo.protocol);
            if (!succ)
            {
                _pending_accept_sockets.insert(tcp_socket);
                ++_ovl_ref;
            }
        }
#endif
    });
}

#ifdef _WIN32
void TcpSocketListener::on_complete(LPWSAOVERLAPPED ovl, size_t len)
{
    --_ovl_ref;

    auto tcp_socket = TcpSocket::get_accept_ex_socket(ovl);
    tcp_socket->accept_ex_complete();
    _pending_accept_sockets.erase(tcp_socket);

    if (_fd == INVALID_SOCKET)
        return;

    for (;;)
    {
        on_accept(this, tcp_socket.get());

        tcp_socket = TcpSocket::create();
        BOOL ret = tcp_socket->accept_ex(_fd, _local_sockinfo.family, _local_sockinfo.socktype, _local_sockinfo.protocol);
        if (!ret)
            break;
    }

    _pending_accept_sockets.insert(tcp_socket);
    ++_ovl_ref;
}

#else // _WIN32
void TcpSocketListener::on_read(size_t len)
{
    for (;;)
    {
        auto socket = accept();
        if (!socket)
            break;

        auto tcp_socket = TcpSocket::create(socket.detach());
        on_accept(this, tcp_socket.get());
    }
}

#endif // _WIN32
