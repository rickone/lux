#pragma once

#include <unordered_set>
#include <unordered_map>
#include "socket.h"

class SocketManager final
{
public:
    SocketManager();
    virtual ~SocketManager();

    static SocketManager * inst();

    void init();
    void clear();
    void on_fork(int pid);
    void gc();

    void add_event(Socket *socket, int event_flag);
    void set_event(Socket *socket, int event_flag);
    void remove_event(Socket *socket) noexcept;
    void wait_event(int timeout);

    template<typename T, typename... A>
    std::shared_ptr<T> create(A... args);

    size_t socket_count() const { return _sockets.size(); }

#ifdef _WIN32
    BOOL accept_ex(socket_t listen_fd, socket_t accept_fd, void *buffer, DWORD local_addr_len, DWORD remote_addr_len, LPOVERLAPPED ovl);
    void get_accept_ex_sockaddrs(void *buffer, DWORD local_addr_len, DWORD remote_addr_len,
        struct sockaddr **local_sockaddr, socklen_t *local_sockaddr_len, struct sockaddr **remote_sockaddr, socklen_t *remote_sockaddr_len);
    BOOL connect_ex(socket_t fd, const struct sockaddr *addr, socklen_t addrlen, LPOVERLAPPED ovl);
#endif

private:
#ifdef _WIN32
    HANDLE _hd = NULL;

    LPFN_ACCEPTEX _fn_accept_ex = NULL;
    LPFN_GETACCEPTEXSOCKADDRS _fn_get_accept_ex_sockaddrs = NULL;
    LPFN_CONNECTEX _fn_connect_ex = NULL;

    // lost socket: main thread closed but IOCP didn't complete yet
    std::unordered_set< std::shared_ptr<Socket> > _lost_sockets;
#else
    int _fd = -1;
#endif
    std::unordered_map<int, std::shared_ptr<Socket> > _sockets;
    int _next_socket_id;
};

template<typename T, typename... A>
std::shared_ptr<T> SocketManager::create(A... args)
{
    static_assert(std::is_base_of<Socket, T>::value, "SocketManager::create failed, need a Socket-based type");

    auto socket = std::make_shared<T>(args...);
    int id = ++_next_socket_id;
    socket->set_id(id);

    _sockets.insert(std::make_pair(id, socket));
    return socket;
}
