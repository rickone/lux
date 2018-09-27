#include "socket.h"
#include "socket_manager.h"

Socket::Socket() : _fd(INVALID_SOCKET)
#ifdef _WIN32
    , _read_ovl(), _write_ovl(), _ovl_ref()
#endif
{
}

Socket::Socket(socket_t fd) : _fd(fd)
#ifdef _WIN32
    , _read_ovl(), _write_ovl(), _ovl_ref()
#endif
{
}

Socket::Socket(int domain, int type, int protocol) : _fd(INVALID_SOCKET)
#ifdef _WIN32
    , _read_ovl(), _write_ovl(), _ovl_ref()
#endif
{
    init(domain, type, protocol);
}

Socket::Socket(Socket&& socket) : _fd(socket.detach())
#ifdef _WIN32
    , _read_ovl(), _write_ovl(), _ovl_ref()
#endif
{
}

Socket::~Socket()
{
    close();
}

void Socket::new_class(lua_State *L)
{
    lua_new_class(L, Socket);

    lua_newtable(L);
    {
        lua_method(L, close);
        lua_std_method(L, connect);
        lua_std_method(L, send);
        lua_std_method(L, sendto);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_property_readonly(L, id);
        lua_property_readonly(L, fd);

        lua_callback(L, on_connect);
        lua_callback(L, on_close);
        lua_callback(L, on_accept);
        lua_callback(L, on_recv);
        lua_callback(L, on_recvfrom);
        lua_callback(L, on_error);
    }
    lua_setfield(L, -2, "__property");
}

Socket& Socket::operator =(Socket &&socket)
{
    socket_t fd = socket.detach();
    attach(fd);
    return *this;
}

void Socket::init(int family, int socktype, int protocol)
{
    logic_assert(_fd == INVALID_SOCKET, "_fd = %d", _fd);

#ifdef __linux__
    _fd = ::socket(family, socktype | SOCK_NONBLOCK, protocol);
#else
    _fd = ::socket(family, socktype, protocol);
#endif
    if (_fd == INVALID_SOCKET)
        throw_socket_error();

#if !defined(__linux__)
    set_nonblock();
#endif

    log_info("create socket(%d, %d, %d) fd(%d)", family, socktype, protocol, _fd);
}

void Socket::close() noexcept
{
    if (_fd != INVALID_SOCKET)
    {
        log_info("close fd(%d)", _fd);
#ifdef _WIN32
        closesocket(_fd);
#else
        ::close(_fd);
#endif
        _fd = INVALID_SOCKET;
    }
}

void Socket::attach(socket_t fd)
{
    close();
    _fd = fd;
}

socket_t Socket::detach()
{
    socket_t fd = _fd;
    _fd = INVALID_SOCKET;
    return fd;
}

void Socket::add_event(int event_flag)
{
    SocketManager::inst()->add_event(this, event_flag);
}

void Socket::set_event(int event_flag)
{
    SocketManager::inst()->set_event(this, event_flag);
}

void Socket::set_nonblock()
{
#ifdef _WIN32
    u_long  opt = 1;
    ioctlsocket(_fd, FIONBIO, &opt);
#else
    int option = fcntl(_fd, F_GETFL);
    if (option < 0)
        throw_socket_error();
    
    int ret = fcntl(_fd, F_SETFL, option | O_NONBLOCK);
    if (ret != 0)
        throw_socket_error();
#endif
}

void Socket::bind(const struct sockaddr *addr, socklen_t addrlen)
{
    int ret = ::bind(_fd, addr, addrlen);
    if (ret != 0)
        throw_socket_error();

    auto name = get_addrname(addr, addrlen);
    log_info("fd(%d) bind to %s", _fd, name.c_str());
}

void Socket::bind_any(int family)
{
    if (family == AF_INET)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = 0;

        bind((const sockaddr *)&addr, sizeof(addr));
        return;
    }

    if (family == AF_INET6)
    {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = in6addr_any;
        addr.sin6_port = 0;

        bind((const sockaddr *)&addr, sizeof(addr));
        return;
    }

    throw_error(std::runtime_error, "unsupported address family: %u", family);
}

bool Socket::connect(const struct sockaddr *addr, socklen_t addrlen)
{
    int ret = ::connect(_fd, addr, addrlen);
    if (ret != 0)
    {
        int err = get_socket_error();
        if (err != EINPROGRESS)
            throw_socket_error();
    }

    auto name = get_addrname(addr, addrlen);
    bool connected = (ret == 0);
    log_info("fd(%d) connect %s ... %s", _fd, name.c_str(), (connected ? "OK" : "pending"));
    return connected;
}

void Socket::listen(int backlog)
{
    int ret = ::listen(_fd, backlog);
    if (ret != 0)
        throw_socket_error();

    log_info("fd(%d) listen backlog(%d)", _fd, backlog);
}

void Socket::setsockopt(int level, int optname, bool enable)
{
    int int_val = enable ? 1 : 0;
    socklen_t int_len = sizeof(int);

    int ret = ::setsockopt(_fd, level, optname, (const char *)&int_val, int_len);
    if (ret != 0)
        throw_socket_error();
}

void Socket::getsockopt(int level, int optname, int *out_value)
{
    socklen_t int_len = sizeof(int);

    int ret = ::getsockopt(_fd, level, optname, (char *)&out_value, &int_len);
    if (ret != 0)
        throw_socket_error();
}

Socket Socket::accept()
{
    struct sockaddr_storage addr;
    socklen_t addrlen = (socklen_t)sizeof(addr);

#ifdef __linux__
    socket_t fd = accept4(_fd, (struct sockaddr *)&addr, &addrlen, O_NONBLOCK);
#else
    socket_t fd = ::accept(_fd, (struct sockaddr *)&addr, &addrlen);
#endif
    if (fd == INVALID_SOCKET)
    {
        int err = get_socket_error();
        if (err != EWOULDBLOCK)
            throw_socket_error();

        return Socket();
    }

    Socket socket(fd);

#if !defined(__linux__)
    socket.set_nonblock();
#endif

    auto name = get_addrname((const struct sockaddr *)&addr, addrlen);
    log_info("fd(%d) accept fd(%d) from %s", _fd, fd, name.c_str());
    return socket;
}

int Socket::recvfrom(char *data, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    int ret = ::recvfrom(_fd, data, len, flags, src_addr, addrlen);
    if (ret == SOCKET_ERROR)
    {
        int err = get_socket_error();
        if (err != EWOULDBLOCK)
            throw_socket_error();

        return -1;
    }

#ifdef _DEBUG
    if (src_addr)
    {
        auto name = get_addrname(src_addr, *addrlen);
        log_debug("fd(%d) recv %d bytes from %s", _fd, ret, name.c_str());
    }
    else
    {
        log_debug("fd(%d) recv %d bytes", _fd, ret);
    }
#endif
    return ret;
}

int Socket::sendto(const char *data, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    int ret = ::sendto(_fd, data, len, flags, dest_addr, addrlen);
    if (ret == SOCKET_ERROR)
    {
        int err = get_socket_error();
        if (err != EWOULDBLOCK)
            throw_socket_error();

        return -1;
    }

#ifdef _DEBUG
    if (dest_addr)
    {
        auto name = get_addrname(dest_addr, addrlen);
        log_debug("fd(%d) send %d bytes to %s", _fd, ret, name.c_str());
    }
    else
    {
        log_debug("fd(%d) send %d bytes", _fd, ret);
    }
#endif
    return ret;
}

int Socket::recv(char *data, size_t len, int flags)
{
    int ret = ::recv(_fd, data, len, flags);
    if (ret == SOCKET_ERROR)
    {
        int err = get_socket_error();
        if (err != EWOULDBLOCK)
            throw_socket_error();

        return -1;
    }

    log_debug("fd(%d) recv %d bytes", _fd, ret);
    return ret;
}

int Socket::send(const char *data, size_t len, int flags)
{
    int ret = ::send(_fd, data, len, flags);
    if (ret < 0)
    {
        int err = get_socket_error();
        if (err != EWOULDBLOCK)
            throw_socket_error();

        return -1;
    }

    log_debug("fd(%d) send %d bytes", _fd, ret);
    return ret;
}

#ifndef _WIN32
int Socket::read(char *data, size_t len)
{
    int ret = ::read(_fd, data, len);
    if (ret < 0)
    {
        if (errno != EAGAIN)
            throw_socket_error();

        return -1;
    }

    log_debug("fd(%d) read %d bytes", _fd, ret);
    return ret;
}

int Socket::write(const char *data, size_t len)
{
    int ret = ::write(_fd, data, len);
    if (ret < 0)
    {
        if (errno != EAGAIN)
            throw_socket_error();

        return -1;
    }

    log_debug("fd(%d) write %d bytes", _fd, ret);
    return ret;
}
#else // _WIN32
int Socket::wsa_recvfrom(char *data, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    WSABUF wsa_buf = { (ULONG)len, data };
    DWORD recv_len = 0;

    memset(&_read_ovl, 0, sizeof(_read_ovl));

    int ret = WSARecvFrom(_fd, &wsa_buf, 1, &recv_len, (LPDWORD)&flags, src_addr, addrlen, &_read_ovl, NULL);
    if (ret == SOCKET_ERROR)
    {
        int err = get_socket_error();
        if (err != WSA_IO_PENDING)
            throw_socket_error();

        ++_ovl_ref;
        return -1;
    }

#ifdef _DEBUG
    if (src_addr)
    {
        auto name = get_addrname(src_addr, *addrlen);
        log_debug("fd(%d) recv %u bytes from %s", _fd, recv_len, name.c_str());
    }
    else
    {
        log_debug("fd(%d) recv %u bytes", _fd, recv_len);
    }
#endif
    return (int)recv_len;
}

int Socket::wsa_sendto(const char *data, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    WSABUF wsa_buf = { (ULONG)len, const_cast<char *>(data) };
    DWORD send_len = 0;

    memset(&_write_ovl, 0, sizeof(_write_ovl));

    int ret = WSASendTo(_fd, &wsa_buf, 1, &send_len, (DWORD)flags, dest_addr, addrlen, &_write_ovl, NULL);
    if (ret == SOCKET_ERROR)
    {
        int err = get_socket_error();
        if (err != WSA_IO_PENDING)
            throw_socket_error();

        ++_ovl_ref;
        return -1;
    }

#ifdef _DEBUG
    if (dest_addr)
    {
        auto name = get_addrname(dest_addr, addrlen);
        log_debug("fd(%d) send %d bytes to %s", _fd, ret, name.c_str());
    }
    else
    {
        log_debug("fd(%d) send %d bytes", _fd, ret);
    }
#endif
    return (int)send_len;
}

int Socket::wsa_recv(char *data, size_t len, int flags)
{
    WSABUF wsa_buf = { (ULONG)len, data };
    DWORD recv_len = 0;

    memset(&_read_ovl, 0, sizeof(_read_ovl));

    int ret = WSARecv(_fd, &wsa_buf, 1, &recv_len, (LPDWORD)&flags, &_read_ovl, NULL);
    if (ret == SOCKET_ERROR)
    {
        int err = get_socket_error();
        if (err != WSA_IO_PENDING)
            throw_socket_error();

        ++_ovl_ref;
        return -1;
    }

    log_debug("fd(%d) recv %u bytes", _fd, recv_len);
    return (int)recv_len;
}

int Socket::wsa_send(const char *data, size_t len, int flags)
{
    WSABUF wsa_buf = { (ULONG)len, const_cast<char *>(data) };
    DWORD send_len = 0;

    memset(&_write_ovl, 0, sizeof(_write_ovl));

    int ret = WSASend(_fd, &wsa_buf, 1, &send_len, (DWORD)flags, &_write_ovl, NULL);
    if (ret == SOCKET_ERROR)
    {
        int err = get_socket_error();
        if (err != WSA_IO_PENDING)
            throw_socket_error();

        ++_ovl_ref;
        return -1;
    }

    log_debug("fd(%d) send %u bytes", _fd, send_len);
    return (int)send_len;
}
#endif

int Socket::lua_connect(lua_State *L)
{
    size_t addrlen = 0;
    struct sockaddr *addr = (struct sockaddr *)luaL_checklstring(L, 1, &addrlen);

    connect(addr, (socklen_t)addrlen);
    return 0;
}

int Socket::lua_send(lua_State *L)
{
    size_t len = 0;
    const char *data = luaL_checklstring(L, 1, &len);

    send(data, len, 0);
    return 0;
}

int Socket::lua_sendto(lua_State *L)
{
    size_t len = 0;
    const char *data = luaL_checklstring(L, 1, &len);
    size_t addrlen = 0;
    struct sockaddr *addr = (struct sockaddr *)luaL_checklstring(L, 2, &addrlen);

    sendto(data, len, 0, addr, (socklen_t)addrlen);
    return 0;
}

void Socket::on_read(size_t len)
{
}

void Socket::on_write(size_t len)
{
}

#ifdef _WIN32
void Socket::on_complete(LPWSAOVERLAPPED ovl, size_t len)
{
    --_ovl_ref;
    if (_fd == INVALID_SOCKET)
        return;

    if (ovl == &_read_ovl)
        on_read(len);
    else if (ovl == &_write_ovl)
        on_write(len);
}
#endif
