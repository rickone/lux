#include "socket_kcp.h"
#include "log.h"
#include "entity.h"
#include "timer_manager.h"

static int kcp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    SocketKcp *sr = (SocketKcp *)user;
    return sr->socket_send(buf, len);
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
        lua_std_method(L, send);
    }
    lua_setfield(L, -2, "__method");

    lua_lib(L, "lux_core");
    {
        lua_set_method(L, "create_kcp", create);
    }
    lua_pop(L, 1);
}

std::shared_ptr<SocketKcp> SocketKcp::create()
{
    return std::shared_ptr<SocketKcp>(new SocketKcp());
}

int SocketKcp::socket_send(const char *data, size_t len)
{
    _socket->send(data, len, 0);
    return 0;
}

void SocketKcp::on_timer(Timer *timer)
{
    if (_kcp == nullptr)
    {
        timer->stop();
        return;
    }

    unsigned int time = TimerManager::inst()->time_now();
    unsigned int update_time = ikcp_check(_kcp, time);
    if (time >= update_time)
        ikcp_update(_kcp, time);
}

void SocketKcp::on_socket_recv(Socket *socket, Buffer *buffer, LuaSockAddr *saddr)
{
    while (!buffer->empty())
    {
        auto front = buffer->front();
        int ret = ikcp_input(_kcp, front.first, front.second);
        if (ret != 0)
        {
            _entity->remove();
            throw_error(std::runtime_error, "ikcp_input buffer(%u) retcode(%d)", buffer->size(), ret);
        }

        buffer->pop(nullptr, front.second);
    }

    while (_kcp)
    {
        _recv_buffer.clear();

        auto back = _recv_buffer.back();
        int recv_len = ikcp_recv(_kcp, back.first, back.second);
        if (recv_len < 0)
            break;

        _recv_buffer.push(nullptr, recv_len);

        _callback(this, &_recv_buffer);
    }
}

void SocketKcp::send(const char *data, size_t len)
{
    int ret = ikcp_send(_kcp, data, len);
    if (ret < 0)
    {
        _entity->remove();
        throw_error(std::runtime_error, "ikcp_send buffer(%u) retcode(%d)", len, ret);
    }
}

int SocketKcp::lua_send(lua_State *L)
{
    size_t len = 0;
    const char *data = luaL_checklstring(L, 1, &len);

    send(data, len);
    return 0;
}

void SocketKcp::start()
{
    _kcp = ikcp_create(0x1985, this);
    _kcp->output = kcp_output;
    _kcp->writelog = kcp_writelog;
    //_kcp->logmask = IKCP_LOG_INPUT | IKCP_LOG_RECV;
    ikcp_wndsize(_kcp, 128, 128);
    ikcp_nodelay(_kcp, 1, 10, 2, 1);
    //ikcp_setmtu(_kcp, 512);

    auto timer = _entity->add_timer(20);
    timer->on_timer.set(this, &SocketKcp::on_timer);

    _socket = std::static_pointer_cast<Socket>(_entity->find_component("socket"));
    _socket->on_recvfrom.set(this, &SocketKcp::on_socket_recv);
}

void SocketKcp::stop() noexcept
{
    if (_kcp)
    {
        ikcp_release(_kcp);
        _kcp = nullptr;
    }
}
