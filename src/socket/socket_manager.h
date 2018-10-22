#pragma once

#include <unordered_set>
#include <unordered_map>
#include "socket.h"

namespace lux {

class SocketManager final
{
public:
    SocketManager();
    virtual ~SocketManager();

    static SocketManager *inst();

    void init();
    void clear();
    void on_fork(int pid);

    void add_event(Socket *socket, int event_flag);
    void set_event(Socket *socket, int event_flag);
    void remove_event(Socket *socket) noexcept;
    void wait_event(int timeout);

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
#else
    int _fd = -1;
#endif
};

} // lux
