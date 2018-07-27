#pragma once
#if !defined(_WIN32)

#include "unix_socket.h"

class UnixSocketStream : public UnixSocket
{
public:
    UnixSocketStream() = default;
    virtual ~UnixSocketStream();

    static void new_class(lua_State *L);
    static std::pair<Socket, Socket> create_pair();
    static std::shared_ptr<UnixSocketStream> create(int fd);
    static std::shared_ptr<UnixSocketStream> connect(const char *socket_path);
    static std::shared_ptr<UnixSocketStream> fork(const char *proc_title, std::function<void (const std::shared_ptr<UnixSocketStream> &socket)> worker_proc);
    static int lua_fork(lua_State *L);

    void init_connect(const char *socket_path);

    virtual int send(const char *data, size_t len, int flags) override;
    virtual void on_read(size_t len) override;
    virtual void on_write(size_t len) override;

protected:
    void flush();
    void on_recvfrom(size_t len);
    void on_recv(size_t len);

    Buffer _recv_buffer;
    Buffer _send_buffer;
    std::list<int> _recv_fd_list;
    std::list<int> _send_fd_list;
    void (UnixSocketStream::*_on_read)(size_t len);
    std::string _socket_path;
};

#endif // !_WIN32
