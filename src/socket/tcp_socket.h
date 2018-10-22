#pragma once

#include <memory>
#include "socket.h"
#include "socket_package.h"

namespace lux {

class TcpSocket : public Socket
{
public:
    TcpSocket();
    virtual ~TcpSocket() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<TcpSocket> create(socket_t fd = INVALID_SOCKET);
    static std::shared_ptr<TcpSocket> connect(const char *node, const char *service);

    void init_connection(const char *node, const char *service);
    void send_pending(const char *data, size_t len);
    void sendv(RawBuffer *rb, size_t count);
    void on_recv_buffer(Buffer *buffer);
    void set_package_mode();

    virtual int send(const char *data, size_t len, int flags) override;
    virtual void on_read(size_t len) override;
    virtual void on_write(size_t len) override;

#ifdef _WIN32
    static std::shared_ptr<TcpSocket> get_accept_ex_socket(LPWSAOVERLAPPED ovl);

    BOOL accept_ex(socket_t listen_fd, int family, int socktype, int protocol);
    void accept_ex_complete();
    BOOL connect_ex(const struct sockaddr *addr, socklen_t addrlen);
#endif

protected:
    void flush();

    bool _connected = false;
    Buffer _recv_buffer;
    Buffer _send_buffer;

    std::shared_ptr<SocketPackage> _package;

#ifdef _WIN32
#define ACCEPT_EX_ADDRESS_LEN (64) // at least 44 = sizeof(sockaddr_in6) + 16

    char _accept_ex_buffer[ACCEPT_EX_ADDRESS_LEN * 2];
#endif
};

} // lux
