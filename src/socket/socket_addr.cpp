#include "socket_addr.h"
#include <signal.h>
#include <sys/signalfd.h>

SocketAddr::SocketAddr()
{
    memset(&_gaicb, 0, sizeof(_gaicb));
}

SocketAddr::~SocketAddr()
{
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
}

std::shared_ptr<SocketAddr> SocketAddr::create(const char *node, const char *service, int socktype, int flags)
{
    std::shared_ptr<SocketAddr> sa(new SocketAddr());
    sa->init(node, service, socktype, flags);
    return sa;
}

void SocketAddr::init(const char *node, const char *service, int socktype, int flags)
{
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
}

struct addrinfo * SocketAddr::first_addrinfo()
{
    if (!_gaicb.ar_result)
        return nullptr;

    _cur_addrinfo = _gaicb.ar_result;
    return _cur_addrinfo;
}

struct addrinfo * SocketAddr::next_addrinfo()
{
    if (!_cur_addrinfo)
        return nullptr;

    _cur_addrinfo = _cur_addrinfo->ai_next;
    return _cur_addrinfo;
}

void SocketAddr::on_read(size_t len)
{
    struct signalfd_siginfo siginfo;

    int ret = read((char *)&siginfo, sizeof(siginfo));
    if (ret < (int)sizeof(siginfo))
        throw_socket_error();

    _callback(this);

    close();
}
