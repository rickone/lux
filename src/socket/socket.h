#pragma once

#include "socket_def.h"
#include "socket_utils.h"
#include "callback.h"
#include "buffer.h"

enum SocketEventFlag
{
    kSocketEvent_None       = 0,
    kSocketEvent_Read       = 1,
    kSocketEvent_Write      = 2,
    kSocketEvent_ReadWrite  = 3,
};

class Socket : public LuaObject
{
public:
    Socket();
    explicit Socket(socket_t fd);
    Socket(int domain, int type, int protocol);
    Socket(const Socket& sock) = delete;
    Socket(Socket&& sock);
    virtual ~Socket();

    static void new_class(lua_State *L);

    Socket& operator =(const Socket &sock) = delete;
    Socket& operator =(Socket &&sock);

    void init(int family, int socktype, int protocol);
    void close() noexcept;
    void attach(socket_t fd);
    socket_t detach();
    void add_event(int event_flag);
    void set_event(int event_flag);
    void set_nonblock();

    void bind(const struct sockaddr *addr, socklen_t addrlen);
    void bind_any(int family);
    bool connect(const struct sockaddr *addr, socklen_t addrlen);
    void listen(int backlog);
    void setsockopt(int level, int optname, bool enable);
    void getsockopt(int level, int optname, int *out_value);

    Socket accept();

    virtual int recvfrom(char *data, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    virtual int sendto(const char *data, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
    virtual int recv(char *data, size_t len, int flags);
    virtual int send(const char *data, size_t len, int flags);
#ifndef _WIN32
    virtual int read(char *data, size_t len);
    virtual int write(const char *data, size_t len);
#else
    int wsa_recvfrom(char *data, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    int wsa_sendto(const char *data, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
    int wsa_recv(char *data, size_t len, int flags);
    int wsa_send(const char *data, size_t len, int flags);
#endif

    int lua_connect(lua_State *L);
    int lua_send(lua_State *L);
    int lua_sendto(lua_State *L);
    
    virtual void on_read(size_t len);
    virtual void on_write(size_t len);
#ifdef _WIN32
    virtual void on_complete(LPWSAOVERLAPPED ovl, size_t len);
#endif

    int id() const { return _id; }
    void set_id(int id) { _id = id; }

    socket_t fd() const { return _fd; }

#ifdef _WIN32
    operator bool() const { return _fd != INVALID_SOCKET || _ovl_ref > 0; }
#else
    operator bool() const { return _fd != INVALID_SOCKET; }
#endif

    def_lua_callback(on_connect, Socket *)
    def_lua_callback(on_close, Socket *)
    def_lua_callback(on_accept, Socket *, Socket *)
    def_lua_callback(on_recv, Socket *, Buffer *)
    def_lua_callback(on_recvfrom, Socket *, Buffer *, RawData *)
    def_lua_callback(on_error, Socket *, int)
    
protected:
    int _id;
    socket_t _fd;

#ifdef _WIN32
    WSAOVERLAPPED _read_ovl;
    WSAOVERLAPPED _write_ovl;
    int _ovl_ref;
#endif
};

#ifdef _WIN32
#define throw_socket_error() \
    do  \
    {   \
        socket_t fd = _fd;   \
        int __sock_err = WSAGetLastError(); \
        on_error(this, __sock_err);     \
        close(); \
        throw_system_error(__sock_err, win32_category(), "fd(%d)", (int)fd); \
    } while (false)
#else
#define throw_socket_error() \
    do  \
    {   \
        socket_t fd = _fd;   \
        on_error(this, errno);     \
        close(); \
        throw_unix_error("fd(%d)", (int)fd); \
    } while (false)
#endif
