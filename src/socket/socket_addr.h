#pragma once

#include <memory>
#include "socket.h"

class SocketAddr : public Socket
{
public:
    SocketAddr();
    virtual ~SocketAddr();

    static std::shared_ptr<SocketAddr> create(const char *node, const char *service, int socktype, int flags);

    void init(const char *node, const char *service, int socktype, int flags);
    struct addrinfo * first_addrinfo();
    struct addrinfo * next_addrinfo();

    virtual void on_read(size_t len) override;

    template<typename T, typename F>
    void set_callback(T *object, F func) { _callback.set(object, func); }

private:
    struct gaicb _gaicb;
    struct addrinfo *_cur_addrinfo;
    Callback<SocketAddr *> _callback;
};
