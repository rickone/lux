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
    struct addrinfo * result();

    virtual void on_read(size_t len) override;

    def_lua_callback(on_result, SocketAddr *)

private:
#ifdef __linux__
    struct gaicb _gaicb;
#else
    struct addrinfo *_ai_result = nullptr;
#endif
};
