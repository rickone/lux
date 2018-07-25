#pragma once
#if !defined(_WIN32)

#include <memory>
#include <list>
#include "socket.h"
#include "buffer.h"

class UnixSocket : public Socket
{
public:
    UnixSocket() = default;
    virtual ~UnixSocket();

    static void new_class(lua_State *L);
    static std::pair<Socket, Socket> create_pair();
    static std::shared_ptr<UnixSocket> create(int fd);
    static std::shared_ptr<UnixSocket> new_service(const char *socket_path);
    static std::shared_ptr<UnixSocket> connect(const char *socket_path);
    static std::shared_ptr<UnixSocket> fork(const char *proc_title, std::function<void (const std::shared_ptr<UnixSocket> &socket)> worker_proc);
    static int lua_fork(lua_State *L);

    void init_service(const char *socket_path);
    void init_connect(const char *socket_path);

    int recv(void *data, size_t len, int flags);
    int send(const void *data, size_t len, int flags);
    void push_fd(int fd);
    int pop_fd();

    virtual void on_read(size_t len) override;
    virtual void on_write(size_t len) override;
    virtual void send_data(const char *data, size_t len) override;

protected:
    void flush();
    void on_accept();
    void on_recv();
    void on_send();

    Buffer _recv_buffer;
    Buffer _send_buffer;
    std::list<int> _recv_fd_list;
    std::list<int> _send_fd_list;
    void (UnixSocket::*_on_read)();
    void (UnixSocket::*_on_write)();
    std::string _socket_path;
};

#endif // !_WIN32
