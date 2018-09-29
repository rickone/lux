#pragma once

#include <memory>
#include <unordered_set>
#include "tcp_socket.h"
#include "buffer.h"

class TcpSocketListener : public Socket
{
public:
    TcpSocketListener() = default;
    virtual ~TcpSocketListener() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<TcpSocketListener> create(const char *node, const char *service);

    void init_service(const char *node, const char *service);
    void set_package_mode();

#ifdef _WIN32
    virtual void on_complete(LPWSAOVERLAPPED ovl, size_t len) override;
#else
    virtual void on_read(size_t len) override;
#endif

private:
#ifdef _WIN32
    struct {
        int family;
        int socktype;
        int protocol;
    } _local_sockinfo;

    std::unordered_set< std::shared_ptr<TcpSocket> > _pending_accept_sockets;
#endif
    bool _package_mode = false;
};
