#include "socket_kcp.h"
#include "timer_manager.h"
#include "log.h"

using namespace lux;

static int kcp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    SocketKcp *sr = (SocketKcp *)user;
    RawBuffer rb;
    rb.data = buf;
    rb.len = len;
    sr->on_send(&rb);
    return 0;
}

static void kcp_writelog(const char *log, struct IKCPCB *kcp, void *user)
{
    log_info("[KCP] %s", log);
}

SocketKcp::~SocketKcp()
{
    if (_kcp)
    {
        ikcp_release(_kcp);
        _kcp = nullptr;
    }
}

void SocketKcp::new_class(lua_State *L)
{
    lua_new_class(L, SocketKcp);

    lua_newtable(L);
    {
        lua_std_method(L, recv);
        lua_std_method(L, send);
    }
    lua_setfield(L, -2, "__method");

    lua_newtable(L);
    {
        lua_callback(L, on_recv);
        lua_callback(L, on_send);
        lua_callback(L, on_error);
    }
    lua_setfield(L, -2, "__property");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_kcp", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<SocketKcp> SocketKcp::create()
{
    auto kcp = std::make_shared<SocketKcp>();
    kcp->init();
    return kcp;
}

void SocketKcp::init()
{
    _kcp = ikcp_create(0x1985, this);
    _kcp->output = kcp_output;
    _kcp->writelog = kcp_writelog;
    //_kcp->logmask = IKCP_LOG_INPUT | IKCP_LOG_RECV;
    ikcp_wndsize(_kcp, 128, 128);
    ikcp_nodelay(_kcp, 1, 10, 2, 1);
    //ikcp_setmtu(_kcp, 512);

    _timer = Timer::create(20);
    _timer->on_timer.set(this, &SocketKcp::on_timer);
}

void SocketKcp::recv(const char *data, size_t len)
{
    int ret = ikcp_input(_kcp, data, len);
    if (ret != 0)
    {
        on_error(this, ret);
        throw_error(std::runtime_error, "ikcp_input buffer(%u) retcode(%d)", len, ret);
    }

    while (_kcp)
    {
        _recv_buffer.clear();

        auto back = _recv_buffer.back();
        int recv_len = ikcp_recv(_kcp, back.first, back.second);
        if (recv_len < 0)
            break;

        _recv_buffer.push(nullptr, recv_len);

        on_recv(&_recv_buffer);
    }
}

void SocketKcp::send(const char *data, size_t len)
{
    int ret = ikcp_send(_kcp, data, len);
    if (ret < 0)
    {
        on_error(this, ret);
        throw_error(std::runtime_error, "ikcp_send buffer(%u) retcode(%d)", len, ret);
    }
}

void SocketKcp::on_timer()
{
    if (_kcp == nullptr)
    {
        _timer->clear();
        return;
    }

    unsigned int time = _timer->duration();
    unsigned int update_time = ikcp_check(_kcp, time);
    if (time >= update_time)
        ikcp_update(_kcp, time);
}

int SocketKcp::lua_recv(lua_State *L)
{
    if (lua_isstring(L, 1))
    {
        size_t len = 0;
        const char *data = lua_tolstring(L, 1, &len);
        recv(data, len);
    }
    else
    {
        Buffer *buffer = lua_to_ptr<Buffer>(L, 1);
        while (!buffer->empty())
        {
            auto front = buffer->front();
            recv(front.first, front.second);
            buffer->pop(nullptr, front.second);
        }
    }
    return 0;
}

int SocketKcp::lua_send(lua_State *L)
{
    if (lua_isstring(L, 1))
    {
        size_t len = 0;
        const char *data = lua_tolstring(L, 1, &len);
        send(data, len);
    }
    else
    {
        Buffer *buffer = lua_to_ptr<Buffer>(L, 1);
        while (!buffer->empty())
        {
            auto front = buffer->front();
            send(front.first, front.second);
            buffer->pop(nullptr, front.second);
        }
    }
    return 0;
}
