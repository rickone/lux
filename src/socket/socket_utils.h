#pragma once

#include <memory> // unique_ptr
#include <system_error>
#include <functional>
#include "socket_def.h"
#include "error.h"
#include "log.h"

namespace lux {

struct AddrInfoDeleter
{
    void operator() (addrinfo *ai) const noexcept
    {
        freeaddrinfo(ai);
    }
};

typedef std::unique_ptr<addrinfo, AddrInfoDeleter> addrinfo_uptr;
addrinfo_uptr query_addrinfo(const char *node, const char *service, int ai_socktype, int ai_flags);

void any_addrinfo(const char *node, const char *service, int ai_socktype, int ai_flags, const std::function<void (const struct addrinfo *ai)> &func);

std::string get_addrname(const struct sockaddr *addr, socklen_t addrlen);

} // lux
