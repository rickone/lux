#pragma once
#if !defined(_WIN32)

#include <memory>
#include <list>
#include "socket.h"
#include "buffer.h"
#include "tcp_socket.h"
#include "udp_socket.h"

class UnixSocket : public Socket
{
public:
    UnixSocket() = default;
    virtual ~UnixSocket();

    static void new_class(lua_State *L);
    static std::shared_ptr<UnixSocket> bind(const char *socket_path);

    void init_bind(const char *socket_path);
    void connect(const char *socket_path);

    void push_socket(Socket *socket);
    int pop_fd();
    std::shared_ptr<TcpSocket> pop_tcp_socket();
    std::shared_ptr<UdpSocket> pop_udp_socket();

    virtual int recvfrom(char *data, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) override;
    virtual int sendto(const char *data, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) override;
    virtual int send(const char *data, size_t len, int flags) override;
    virtual void on_read(size_t len) override;

protected:
    void on_recvfrom(size_t len);
    void on_recv(size_t len);

    Buffer _recv_buffer;
    Buffer _send_buffer;
    std::list<int> _recv_fd_list;
    std::list<int> _send_fd_list;
    void (UnixSocket::*_on_read)(size_t len);
    std::string _socket_path;
};

#endif // !_WIN32
