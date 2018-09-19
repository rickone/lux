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

    std::shared_ptr<Socket> create();
    void add_event(Socket *socket, int event_flag);
    void set_event(Socket *socket, int event_flag);
    void remove_event(Socket *socket) noexcept;
    void wait_event(int timeout);

    size_t socket_count() const { return _sockets.size(); }

#ifdef _WIN32
    BOOL accept_ex(socket_t listen_fd, socket_t accept_fd, void *buffer, DWORD local_addr_len, DWORD remote_addr_len, LPOVERLAPPED ovl);
    void get_accept_ex_sockaddrs(void *buffer, DWORD local_addr_len, DWORD remote_addr_len,
        struct sockaddr **local_sockaddr, socklen_t *local_sockaddr_len, struct sockaddr **remote_sockaddr, socklen_t *remote_sockaddr_len);
    BOOL connect_ex(socket_t fd, const struct sockaddr *addr, socklen_t addrlen, LPOVERLAPPED ovl);
    void add_lost_socket(const std::shared_ptr<Socket> &socket);
    void gc();
#endif

private:
#ifdef _WIN32
    HANDLE _hd;

    LPFN_ACCEPTEX _fn_accept_ex;
    LPFN_GETACCEPTEXSOCKADDRS _fn_get_accept_ex_sockaddrs;
    LPFN_CONNECTEX _fn_connect_ex;

    // lost socket: main thread closed but IOCP didn't complete yet
    std::unordered_set< std::shared_ptr<Socket> > _lost_sockets;
#else
    int _fd;
#endif
    std::unordered_map<int, std::shared_ptr<Socket> > _sockets;
    int _next_socket_id;
};
