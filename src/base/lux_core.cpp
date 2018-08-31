#include "lux_core.h"
#include <algorithm> // std::min
#include <signal.h>
#include "world.h"
#include "lua_port.h"
#include "tcp_socket.h"
#include "tcp_socket_listener.h"
#include "udp_socket.h"
#include "udp_socket_listener.h"
#include "unix_socket.h"
#include "unix_socket_stream.h"
#include "unix_socket_listener.h"
#include "socket_kcp.h"
#include "socket_package.h"
#include "config.h"

#ifdef _WIN32
#include <process.h>
#define getpid _getpid

#ifdef min
#undef min
#endif

#else
#include <unistd.h> // getpid, fork
#include <sys/wait.h>
#endif

#ifdef __linux__
#include <sys/prctl.h>
#endif

#ifdef GPERFTOOLS
#include <gperftools/profiler.h>
#endif

static LuxCore *s_inst = nullptr;

extern char **environ;

void on_quit(int sig)
{
    LuxCore::inst()->quit();
}

void on_debug(int sig)
{
    static char cmd[32];
#ifdef __APPLE__
    snprintf(cmd, sizeof(cmd), "lldb -p %d", getpid());
#else
    snprintf(cmd, sizeof(cmd), "sudo gdb attach %d", getpid());
#endif
    int ret = system(cmd);
    log_info("system() ret: %d", ret);
    
    signal(sig, nullptr);
}

LuxCore::LuxCore(int argc, char *argv[]) : _running_flag(true), _argc(argc), _argv(argv), _argv_max_len()
{
    s_inst = this;

#if !defined(_WIN32)
    init_set_proc_title();
#endif

    signal(SIGINT, on_quit); // ctrl + c
    signal(SIGTERM, on_quit); // kill
#if !defined(_WIN32)
    signal(SIGQUIT, on_quit); // ctrl + '\'
    signal(SIGCHLD, SIG_IGN);
#endif
#ifdef _DEBUG
    signal(SIGABRT, on_debug);
    signal(SIGFPE, on_debug);
    signal(SIGSEGV, on_debug);
#endif
}

LuxCore::~LuxCore()
{
    s_inst = nullptr;
}

LuxCore * LuxCore::inst()
{
    return s_inst;
}

void LuxCore::run()
{
    profile_start();

    auto timer_mgr = TimerManager::inst();
    auto socket_mgr = SocketManager::inst();

    while (_running_flag)
    {
        int notick_time = timer_mgr->tick();
        socket_mgr->wait_event(notick_time);
    }

    profile_stop();
}

void LuxCore::set_proc_title(const char *title)
{
    size_t title_len = strlen(title) + 1;
    size_t copy_len = std::min(title_len, _argv_max_len);

    memcpy(_argv[0], title, copy_len);
    _argv[0][copy_len - 1] = '\0';

    _argv[1] = nullptr;
    memset(_argv[0] + copy_len, 0, _argv_max_len - copy_len);

#ifdef __linux__
    prctl(PR_SET_NAME, title);
#endif
}

void LuxCore::on_timer(Timer *timer)
{
    if (lua_state)
        lua_gc(lua_state, LUA_GCSTEP, 0);
    World::inst()->gc();
#ifdef _WIN32
    SocketManager::inst()->gc();
#endif
}

void LuxCore::on_fork(int pid)
{
    LogContext::inst()->on_fork(pid);
    SocketManager::inst()->on_fork(pid);
}

void LuxCore::profile_start()
{
#ifdef GPERFTOOLS
    const char *profile = Config::inst()->get_string("profile");
    if (profile)
    {
        ProfilerStart(profile);
        log_info("profile start %s", profile);
    }
#endif
}

void LuxCore::profile_stop()
{
#ifdef GPERFTOOLS
    const char *profile = Config::inst()->get_string("profile");
    if (profile)
    {
        ProfilerStop();
        log_info("profile stop %s", profile);
    }
#endif
}

void LuxCore::start()
{
    auto config = std::make_shared<Config>();
    config->init(_argc, _argv);
    _entity->add_component(config);

    auto log_ctx = std::make_shared<LogContext>();
    log_ctx->init();
    _entity->add_component(log_ctx);

    auto timer_mgr = std::make_shared<TimerManager>();
    _entity->add_component(timer_mgr);

    auto socket_mgr = std::make_shared<SocketManager>();
    socket_mgr->init();
    _entity->add_component(socket_mgr);

    auto timer = _entity->add_timer(200);
    timer->on_timer.set(this, &LuxCore::on_timer);

    lua_port_init();
    lua_core_init(lua_state);

#if !defined(_WIN32)
    if (Config::env()->daemon)
    {
        log_info("enter daemon mode");

        int ret = daemon(1, 0);
        if (ret == -1)
            throw_system_error(errno, "daemon");
    }
#endif

    log_info("LuxCore(v%s) start running, pid=%d, daemon=%s", CORE_VERSION, getpid(), Config::env()->daemon ? "On" : "Off");
}

void LuxCore::stop() noexcept
{
    lua_port_uninit();

    log_info("LuxCore exit, pid=%d", getpid());
}

void LuxCore::init_set_proc_title()
{
    char *offset = _argv[0];
    size_t environ_len = 0;

    for (int i = 0; i < _argc; ++i)
    {
        if (offset != _argv[i])
            break;

        offset += (strlen(_argv[i]) + 1);
    }
    for (int i = 0; environ[i]; ++i)
    {
        size_t len = strlen(environ[i]) + 1;

        environ_len += len;

        if (offset == environ[i])
            offset += len;
    }

    _argv_max_len = offset - _argv[0];

    char *new_environ = (char *)malloc(environ_len);
    offset = new_environ;

    for (int i = 0; environ[i]; ++i)
    {
        size_t len = strlen(environ[i]) + 1;

        memcpy(offset, environ[i], len);
        environ[i] = offset;
        offset += len;
    }
}

void LuxCore::lua_core_init(lua_State *L)
{
    Config::inst()->copy_to_lua(L);

    const char *lua_path = Config::inst()->get_string("lua_path");
    if (lua_path)
    {
        std::string path(lua_path);
        if (!path.empty() && path.back() != ';')
            path.append(1, ';');

        int top = lua_gettop(L);
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        path.append(lua_tostring(L, -1));
        lua_pushlstring(L, path.data(), path.size());
        lua_setfield(L, -3, "path");
        lua_settop(L, top);
    }

    lua_core_openlibs(L);

    const char *start = Config::inst()->get_string("start");
    logic_assert(start, "config.start is empty");

    int ret = luaL_loadfile(L, start);
    if (ret != LUA_OK)
        luaL_error(L, "loadfile(%s) error: %s", start, lua_tostring(L, -1));

    lua_call(L, 0, 1);
    World::inst()->start_lua_component(L);
}

void LuxCore::lua_core_openlibs(lua_State *L)
{
    lua_class_define<Entity>(L);
    lua_class_define<Component>(L);

    lua_class_define<Timer>(L);
    lua_class_define<Buffer>(L);
    lua_class_define<Socket, Component>(L);
    lua_class_define<TcpSocket, Socket>(L);
    lua_class_define<TcpSocketListener, Socket>(L);
    lua_class_define<UdpSocket, Socket>(L);
#if !defined(_WIN32)
    lua_class_define<UdpSocketListener, Socket>(L);
    lua_class_define<UnixSocket, Socket>(L);
    lua_class_define<UnixSocketStream, UnixSocket>(L);
    lua_class_define<UnixSocketListener, Socket>(L);
#endif
    lua_class_define<SocketKcp, Component>(L);
    lua_class_define<SocketPackage, Component>(L);

    lua_lib(L, "lux_core");
    {
        lua_set_function(L, "pack", luap_pack);
        lua_set_function(L, "unpack", luap_unpack);
        lua_std_function(L, log);
    }
    lua_pop(L, 1);
}
