#include "socket_addr.h"
#include <signal.h>
#ifdef __linux__
#include <sys/signalfd.h>
#endif

SocketAddr::SocketAddr()
{
#ifdef __linux__
    memset(&_gaicb, 0, sizeof(_gaicb));
#else
    _ai_result = nullptr;
#endif
}

SocketAddr::~SocketAddr()
{
#ifdef __linux__
    if (_gaicb.ar_name)
    {
        free(const_cast<char *>(_gaicb.ar_name));
        _gaicb.ar_name = nullptr;
    }

    if (_gaicb.ar_service)
    {
        free(const_cast<char *>(_gaicb.ar_service));
        _gaicb.ar_service = nullptr;
    }

    if (_gaicb.ar_request)
    {
        free(const_cast<struct addrinfo *>(_gaicb.ar_request));
        _gaicb.ar_request = nullptr;
    }

    if (_gaicb.ar_result)
    {
        freeaddrinfo(_gaicb.ar_result);
        _gaicb.ar_result = nullptr;
    }
#else
    if (_ai_result)
    {
        freeaddrinfo(_ai_result);
        _ai_result = nullptr;
    }
#endif
}

std::shared_ptr<SocketAddr> SocketAddr::create(const char *node, const char *service, int socktype, int flags)
{
    std::shared_ptr<SocketAddr> sa(new SocketAddr());
    sa->init(node, service, socktype, flags);
    return sa;
}

void SocketAddr::init(const char *node, const char *service, int socktype, int flags)
{
#ifdef __linux__
    logic_assert(_fd == INVALID_SOCKET, "_fd = %d", _fd);

    if (_gaicb.ar_name)
        throw_error(std::runtime_error, "already queryed");

    _gaicb.ar_name = strdup(node);
    _gaicb.ar_service = strdup(service);

    struct addrinfo *hints = (struct addrinfo *)calloc(1, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = socktype;
    hints->ai_flags = flags;
    _gaicb.ar_request = hints;

    struct sigevent sig;
    sig.sigev_notify = SIGEV_SIGNAL;
    sig.sigev_value.sival_ptr = &_gaicb;
    sig.sigev_signo = SIGRTMIN;

    struct gaicb *gaicb_list = &_gaicb;

    int ret = getaddrinfo_a(GAI_NOWAIT, &gaicb_list, 1, &sig);
    if (ret != 0)
        throw_error(std::runtime_error, "getaddrinfo_a(%s, %s, %d, %d) error: %s", node, service, socktype, flags, gai_strerror(ret));

    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    _fd = signalfd(-1, &mask, 0);
    if (_fd == INVALID_SOCKET)
        throw_system_error(errno, "signalfd");
#else
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    hints.ai_flags = flags;

    int ret = getaddrinfo(node, service, &hints, &_ai_result);
    if (ret != 0)
        throw_error(std::runtime_error, "getaddrinfo(%s, %s, %d, %d) error: %s", node, service, socktype, flags, gai_strerror(ret));

    on_result(this);
#endif
}

struct addrinfo * SocketAddr::result()
{
#ifdef __linux__
    if (_gaicb.ar_result)
        return _gaicb.ar_result;
#else
    if (_ai_result)
        return _ai_result;
#endif
    return nullptr;
}

void SocketAddr::on_read(size_t len)
{
#ifdef __linux__
    struct signalfd_siginfo siginfo;

    int ret = read((char *)&siginfo, sizeof(siginfo));
    if (ret < (int)sizeof(siginfo))
        throw_socket_error();

    on_result(this);

    close();
#endif
}
