#pragma once
#if !defined(_WIN32)

#include <memory>
#include "socket.h"

class UnixSocketListener : public Socket
{
public:
    UnixSocketListener() = default;
    virtual ~UnixSocketListener();

    static void new_class(lua_State *L);
    static std::shared_ptr<UnixSocketListener> create(const char *socket_path);

    void init_service(const char *socket_path);

    virtual void on_read(size_t len) override;

protected:
    std::string _socket_path;
};

#endif // !_WIN32
