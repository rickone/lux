#pragma once

#include <memory>
#include "socket.h"
#include "socket_kcp.h"

namespace lux {

class UdpSocket : public Socket
{
public:
    UdpSocket() = default;
    explicit UdpSocket(socket_t fd);
    virtual ~UdpSocket() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<UdpSocket> create(socket_t fd);
    static std::shared_ptr<UdpSocket> bind(const char *node, const char *service);
    static std::shared_ptr<UdpSocket> connect(const char *node, const char *service);

    void init_bind(const char *node, const char *service);
    void init_connect(const char *node, const char *service);
    void on_recv_buffer(Buffer *buffer);
    void send_rawdata(RawBuffer *rb);
    void set_reliable();

    virtual int send(const char *data, size_t len, int flags) override;
    virtual void on_read(size_t len) override;

private:
    void do_recvfrom(size_t len);
    void do_recv(size_t len);

    Buffer _recv_buffer;
    void (UdpSocket::*_on_read)(size_t len);

    sockaddr_storage _remote_sockaddr;
    socklen_t _remote_sockaddr_len;

    std::shared_ptr<SocketKcp> _kcp;
};

} // lux
