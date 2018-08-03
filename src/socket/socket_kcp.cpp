#include "socket_kcp.h"
#include "log.h"
#include "entity.h"
#include "timer_manager.h"

static int kcp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    SocketKcp *sr = (SocketKcp *)user;
    return sr->do_socket_send(buf, len);
}

static void kcp_writelog(const char *log, struct IKCPCB *kcp, void *user)
{
    log_info("[KCP] %s", log);
}

SocketKcp::SocketKcp() : Component("kcp"), _kcp(), _recv_buffer()
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

void SocketKcp::on_socket_recv(LuaObject *msg_object)
{
    Buffer *buffer = (Buffer *)msg_object;

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

        publish(kMsg_KcpRecv, &_recv_buffer);
    }
}

int SocketKcp::do_socket_send(const char *data, size_t len)
{
    RawBuffer buffer;
    buffer.data = data;
    buffer.len = len;
    publish(kMsg_SocketSend, &buffer);
    return 0;
}

void SocketKcp::on_kcp_send(LuaObject *msg_object)
{
    RawBuffer *buffer = (RawBuffer *)msg_object;

    send(buffer->data, buffer->len);
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

void SocketKcp::start(LuaObject *init_object)
{
    _kcp = ikcp_create(0x1985, this);
    _kcp->output = kcp_output;
    _kcp->writelog = kcp_writelog;
    //_kcp->logmask = IKCP_LOG_INPUT | IKCP_LOG_RECV;
    ikcp_wndsize(_kcp, 128, 128);
    ikcp_nodelay(_kcp, 1, 10, 2, 1);
    //ikcp_setmtu(_kcp, 512);

    set_timer(this, &SocketKcp::on_timer, 20);

    subscribe(kMsg_SocketRecv, &SocketKcp::on_socket_recv);
    subscribe(kMsg_KcpSend, &SocketKcp::on_kcp_send);
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
