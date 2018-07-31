#include "socket_utils.h"
#include <cstring> // memset
#include "error.h"
#include "log.h"

class GaiCategory : public std::error_category
{
public:
    virtual const char* name() const noexcept override;
    virtual std::string message(int ev) const override;
};

const char* GaiCategory::name() const noexcept
{
    return "gai_error";
}

std::string GaiCategory::message(int ev) const
{
    return std::string(gai_strerror(ev));
}

addrinfo_uptr query_addrinfo(const char *node, const char *service, int ai_socktype, int ai_flags)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = ai_socktype;
    hints.ai_flags = ai_flags;

    addrinfo *ptr = nullptr;
    int ret = getaddrinfo(node, service, &hints, &ptr);
    if (ret != 0)
        throw std::system_error(std::error_code(ret, GaiCategory()), make_error_info("in %s at %s:%d getaddrinfo(%s, %s) failed", __FUNC__, __FILE__, __LINE__, node, service));

    return addrinfo_uptr(ptr, AddrInfoDeleter());
}

void any_addrinfo(const char *node, const char *service, int ai_socktype, int ai_flags, const std::function<void (const struct addrinfo *ai)> &func)
{
    auto air = query_addrinfo(node, service, ai_socktype, ai_flags);
    for (const struct addrinfo *ai = air.get(); ai; ai = ai->ai_next)
    {
        try
        {
            func(ai);
        }
        catch (const std::runtime_error &err)
        {
            log_error("%s", err.what());
            continue;
        }

        return;
    }

    throw_error(std::runtime_error, "any_addrinfo('%s', '%s') all failed", node, service);
}

std::string get_addrname(const struct sockaddr *addr, socklen_t addrlen)
{
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    getnameinfo(addr, addrlen, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);

    std::string name;
    name.append(host);
    name.append(":");
    name.append(serv);

    return name;
}
