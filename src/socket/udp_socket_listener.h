#pragma once
#if !defined(_WIN32)

#include <memory>
#include <unordered_map>
#include "udp_socket.h"
#include "buffer.h"

class UdpSocketListener : public Socket
{
public:
    UdpSocketListener() = default;
    explicit UdpSocketListener(socket_t fd);
    virtual ~UdpSocketListener() = default;

    static void new_class(lua_State *L);
    static std::shared_ptr<UdpSocketListener> create(const char *node, const char *service);

    void init_service(const char *node, const char *service);
    void set_reliable();
    void do_accept();

    virtual void on_read(size_t len) override;

private:
    static std::string get_sockaddr_key(const struct sockaddr *addr, socklen_t addrlen);

    struct {
        int family;
        int socktype;
        int protocol;
    } _local_sockinfo;
    
    sockaddr_storage _local_sockaddr;
    socklen_t _local_sockaddr_len;
    sockaddr_storage _remote_sockaddr;
    socklen_t _remote_sockaddr_len;

    Buffer _recv_buffer;
    std::unordered_map< std::string, std::shared_ptr<UdpSocket> > _accepted_sockets;
    bool _reliable = false;
};

#endif // !_WIN32
