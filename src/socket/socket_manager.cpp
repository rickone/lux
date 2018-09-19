#include "socket_manager.h"
#include <cassert>
#include "error.h"

const static int MAX_POLL_EVENT_CNT = 1024;

SocketManager::SocketManager() :
#ifdef _WIN32
    _hd(NULL), _fn_accept_ex(), _fn_get_accept_ex_sockaddrs(), _fn_connect_ex()
#else
    _fd(-1)
#endif
{
#ifdef _WIN32
    WORD version = MAKEWORD(2, 2);
    WSADATA wsa_data;
    WSAStartup(version, &wsa_data);
#endif
}

SocketManager::~SocketManager()
{
    clear();

#ifdef _WIN32
    WSACleanup();
#endif

}

SocketManager * SocketManager::inst()
{
    static SocketManager s_inst;
    return &s_inst;
}

void SocketManager::on_fork(int pid)
{
    clear();
    init();
}

std::shared_ptr<Socket> SocketManager::create()
{
    return nullptr;
}

#ifdef __linux__

#include <sys/epoll.h>

void SocketManager::init()
{
    _fd = epoll_create(1024);
    if (_fd < 0)
        throw_system_error(errno, "epoll_create");
}

void SocketManager::clear()
{
    if (_fd >= 0)
    {
        close(_fd);
        _fd = -1;
    }
}

void SocketManager::add_event(Socket *socket, int event_flag)
{
    int fd = socket->fd();

    struct epoll_event event;
    event.events = EPOLLET;
    event.data.ptr = socket;

    if (event_flag & kSocketEvent_Read)
        event.events |= EPOLLIN;

    if (event_flag & kSocketEvent_Write)
        event.events |= EPOLLOUT;

    int ret = epoll_ctl(_fd, EPOLL_CTL_ADD, fd, &event);
    if (ret != 0)
        throw_system_error(errno, "epoll_ctl");
}

void SocketManager::set_event(Socket *socket, int event_flag)
{
    int fd = socket->fd();

    struct epoll_event event;
    event.events = EPOLLET;
    event.data.ptr = socket;

    if (event_flag & kSocketEvent_Read)
        event.events |= EPOLLIN;

    if (event_flag & kSocketEvent_Write)
        event.events |= EPOLLOUT;

    int ret = epoll_ctl(_fd, EPOLL_CTL_MOD, fd, &event);
    if (ret != 0)
        throw_system_error(errno, "epoll_ctl");
}

void SocketManager::remove_event(Socket *socket) noexcept
{
    int fd = socket->fd();

    epoll_ctl(_fd, EPOLL_CTL_DEL, fd, nullptr);
}

void SocketManager::wait_event(int timeout)
{
    static struct epoll_event events[MAX_POLL_EVENT_CNT];
    
    int event_cnt = epoll_wait(_fd, events, MAX_POLL_EVENT_CNT, timeout);
    if (event_cnt < 0)
    {
        if (errno == EINTR)
            return;

        throw_system_error(errno, "epoll_wait");
    }

    for (int i = 0; i < event_cnt; ++i)
    {
        unsigned event_flags = events[i].events;
        Socket *socket = (Socket *)events[i].data.ptr;

        try
        {
            if (event_flags & EPOLLIN)
                socket->on_read(0);

            if (event_flags & EPOLLOUT)
                socket->on_write(0);
        }
        catch (const std::runtime_error &err)
        {
            log_error("%s", err.what());
        }
    }
}

#endif // __linux

#ifdef __APPLE__

#include <sys/event.h>

void SocketManager::init()
{
    _fd = kqueue();
    if (_fd < 0)
        throw_system_error(errno, "kqueue");
}

void SocketManager::clear()
{
    if (_fd >= 0)
    {
        close(_fd);
        _fd = -1;
    }
}

void SocketManager::add_event(Socket *socket, int event_flag)
{
    set_event(socket, event_flag);
}

void SocketManager::set_event(Socket *socket, int event_flag)
{
    int fd = socket->fd();

    struct kevent events[2];
    EV_SET(&events[0], fd, EVFILT_READ,  EV_ADD | ((event_flag & kSocketEvent_Read) ? EV_ENABLE : EV_DISABLE), 0, 0, socket);
    EV_SET(&events[1], fd, EVFILT_WRITE, EV_ADD | ((event_flag & kSocketEvent_Write) ? EV_ENABLE : EV_DISABLE), 0, 0, socket);

    int ret = kevent(_fd, &events[0], 2, nullptr, 0, nullptr);
    if (ret == -1)
        throw_system_error(errno, "kevent");
}

void SocketManager::remove_event(Socket *socket) noexcept
{
    int fd = socket->fd();

    struct kevent events[2]; 
    EV_SET(&events[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&events[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

    kevent(_fd, &events[0], 2, nullptr, 0, nullptr);
}

void SocketManager::wait_event(int timeout)
{
    static struct kevent events[MAX_POLL_EVENT_CNT];

    struct timespec timeout_time;
    timeout_time.tv_sec = timeout / 1000;
    timeout_time.tv_nsec = (long)((timeout % 1000) * 1000'000); // 1s = 10 ^ 9 ns

    int event_cnt = kevent(_fd, nullptr, 0, events, MAX_POLL_EVENT_CNT, &timeout_time);
    if (event_cnt < 0)
    {
        if (errno == EINTR)
            return;

        throw_system_error(errno, "kevent");
    }

    for (int i = 0; i < event_cnt; ++i)
    {
        unsigned filter = events[i].filter;
        Socket *socket = (Socket *)events[i].udata;

        try
        {
            if (filter == EVFILT_READ)
                socket->on_read(0);
            else if (filter == EVFILT_WRITE)
                socket->on_write(0);
        }
        catch (const std::runtime_error &err)
        {
            log_error("%s", err.what());
        }
    }
}

#endif // __APPLE__

#ifdef _WIN32

void SocketManager::init()
{
    _hd = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (_hd == NULL)
        throw_system_error(GetLastError(), "CreateIoCompletionPort");

    Socket socket(AF_INET, SOCK_STREAM, 0);

    DWORD bytes = 0;
    GUID guid = WSAID_ACCEPTEX;
    int ret = WSAIoctl(socket.fd(), SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &_fn_accept_ex, sizeof(_fn_accept_ex), &bytes, nullptr, nullptr);
    if (ret == SOCKET_ERROR)
        throw_system_error(WSAGetLastError(), "WSAIoctl");

    bytes = 0;
    guid = WSAID_GETACCEPTEXSOCKADDRS;
    ret = WSAIoctl(socket.fd(), SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &_fn_get_accept_ex_sockaddrs, sizeof(_fn_get_accept_ex_sockaddrs), &bytes, nullptr, nullptr);
    if (ret == SOCKET_ERROR)
        throw_system_error(WSAGetLastError(), "WSAIoctl");

    bytes = 0;
    guid = WSAID_CONNECTEX;
    ret = WSAIoctl(socket.fd(), SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &_fn_connect_ex, sizeof(_fn_connect_ex), &bytes, nullptr, nullptr);
    if (ret == SOCKET_ERROR)
        throw_system_error(WSAGetLastError(), "WSAIoctl");
}

void SocketManager::clear()
{
    if (_hd != NULL)
    {
        CloseHandle(_hd);
        _hd = NULL;
    }
}

void SocketManager::add_event(Socket *socket, int event_flag)
{
    socket_t fd = socket->fd();

    HANDLE hd = CreateIoCompletionPort((HANDLE)fd, _hd, (ULONG_PTR)socket, 0);
    if (hd == NULL)
        throw_system_error(GetLastError(), "CreateIoCompletionPort");
}

void SocketManager::set_event(Socket *socket, int event_flag)
{
}

void SocketManager::remove_event(Socket *socket) noexcept
{
}

void SocketManager::wait_event(int timeout)
{
    static OVERLAPPED_ENTRY events[MAX_POLL_EVENT_CNT];
    ULONG event_cnt = 0;
    BOOL succ = GetQueuedCompletionStatusEx(_hd, events, MAX_POLL_EVENT_CNT, &event_cnt, (DWORD)timeout, false);
    if (!succ)
    {
        DWORD err = GetLastError();
        if (err == WAIT_TIMEOUT)
            return;

        throw_system_error(err, "GetQueuedCompletionStatusEx");
    }

    for (ULONG i = 0; i < event_cnt; i++)
    {
        OVERLAPPED_ENTRY& oe = events[i];
        Socket *socket = (Socket *)oe.lpCompletionKey;

        try
        {
            socket->on_complete(oe.lpOverlapped, oe.dwNumberOfBytesTransferred);
        }
        catch (const std::runtime_error &err)
        {
            log_error("%s", err.what());
        }
    }
}

BOOL SocketManager::accept_ex(socket_t listen_fd, socket_t accept_fd, void *buffer, DWORD local_addr_len, DWORD remote_addr_len, LPOVERLAPPED ovl)
{
    memset(ovl, 0, sizeof(*ovl));

    DWORD bytes = 0;
    return _fn_accept_ex(listen_fd, accept_fd, buffer, 0, local_addr_len, remote_addr_len, &bytes, ovl);
}

void SocketManager::get_accept_ex_sockaddrs(void *buffer, DWORD local_addr_len, DWORD remote_addr_len,
    struct sockaddr **local_sockaddr, socklen_t *local_sockaddr_len, struct sockaddr **remote_sockaddr, socklen_t *remote_sockaddr_len)
{
    _fn_get_accept_ex_sockaddrs(buffer, 0, local_addr_len, remote_addr_len,
        local_sockaddr, local_sockaddr_len, remote_sockaddr, remote_sockaddr_len);
}

BOOL SocketManager::connect_ex(socket_t fd, const struct sockaddr *addr, socklen_t addrlen, LPOVERLAPPED ovl)
{
    memset(ovl, 0, sizeof(*ovl));

    return _fn_connect_ex(fd, addr, addrlen, nullptr, 0, nullptr, ovl);
}

void SocketManager::add_lost_socket(const std::shared_ptr<Socket> &socket)
{
    log_debug("add lost socket: %p", socket.get());
    _lost_sockets.insert(socket);
}

void SocketManager::gc()
{
    for (auto it = _lost_sockets.begin(); it != _lost_sockets.end(); )
    {
        auto &socket = *it;
        if (socket->ovl_ref() > 0)
        {
            ++it;
            continue;
        }

        log_debug("remove lost socket: %p", socket.get());
        _lost_sockets.erase(it++);
    }
}

#endif // _WIN32
