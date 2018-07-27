#include "socket_kcp.h"
#include "log.h"
#include "entity.h"
#include "timer_manager.h"

static int kcp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    SocketKcp *sr = (SocketKcp *)user;
    return sr->on_kcp_send(buf, len);
}

static void kcp_writelog(const char *log, struct IKCPCB *kcp, void *user)
{
    log_info("[KCP] %s", log);
}

SocketKcp::SocketKcp() : Component("kcp"), _kcp(), _socket(), _recv_buffer()
{
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
        lua_method(L, send);
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

void SocketKcp::send(const std::string &str)
{
    int ret = ikcp_send(_kcp, str.data(), str.size());
    if (ret < 0)
    {
        _entity->remove();
        throw_error(std::runtime_error, "ikcp_send buffer(%u) retcode(%d)", str.size(), ret);
    }
}

int SocketKcp::on_kcp_send(const char *data, size_t len)
{
    _socket->send(data, len, 0);
    return 0;
}

void SocketKcp::on_recv(LuaObject *msg_object)
{
    Buffer *buffer = (Buffer *)msg_object;

    int ret = ikcp_input(_kcp, buffer->data(), buffer->size());
    if (ret != 0)
    {
        _entity->remove();
        throw_error(std::runtime_error, "ikcp_input buffer(%u) retcode(%d)", buffer->size(), ret);
    }
    buffer->clear();

    while (_kcp)
    {
        _recv_buffer.clear();

        auto back = _recv_buffer.back();
        int recv_len = ikcp_recv(_kcp, back.first, back.second);
        if (recv_len < 0)
            break;

        _recv_buffer.push(nullptr, recv_len);

        publish(kMsg_KcpOutput, &_recv_buffer);
    }
}

void SocketKcp::on_timer(Timer *timer)
{
    if (_kcp == nullptr)
    {
        timer->stop();
        return;
    }

    unsigned int time = timer_manager->time_now();
    unsigned int update_time = ikcp_check(_kcp, time);
    if (time >= update_time)
        ikcp_update(_kcp, time);
}

void SocketKcp::start(LuaObject *init_object)
{
    _kcp = ikcp_create(0x1985, this);
    _kcp->output = kcp_output;
    _kcp->writelog = kcp_writelog;
    //_kcp->logmask = IKCP_LOG_INPUT | IKCP_LOG_RECV;
    ikcp_wndsize(_kcp, 128, 128);
    ikcp_nodelay(_kcp, 1, 10, 2, 1);
    //ikcp_setmtu(_kcp, 512);

    _socket = std::dynamic_pointer_cast<Socket>(_entity->find_component("socket"));
    runtime_assert(_socket, "SocketKcp require 'Socket'");

    set_timer(this, &SocketKcp::on_timer, 20);

    subscribe(kMsg_SocketRecv, &SocketKcp::on_recv);
}

void SocketKcp::stop() noexcept
{
    if (_kcp)
    {
        ikcp_release(_kcp);
        _kcp = nullptr;
    }

    unsubscribe(kMsg_SocketRecv);
}
