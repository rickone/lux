#include "system_manager.h"
#include <cstring>
#include <cstdlib>
#include <algorithm> // min
#include <signal.h>
#include "world.h"
#include "lua_port.h"
#include "lua_component.h"
#include "tcp_socket.h"
#include "tcp_socket_listener.h"
#include "udp_socket.h"
#include "udp_socket_listener.h"
#include "unix_socket.h"
#include "unix_socket_stream.h"
#include "unix_socket_listener.h"
#include "socket_kcp.h"
#include "proto_base.h"
#include "luap_object.h"
#include "resp_object.h"
#include "lux_msgr.h"
#include "config.h"
#include "error.h"

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

SystemManager *system_manager = nullptr;

extern char **environ;

void on_quit(int sig)
{
    system_manager->quit();
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
    log_info("system return: %d", ret);
    
    signal(sig, nullptr);
}

#if !defined(_WIN32)
void on_child_exit(int sig)
{
    int pid, status;

    for (;;)
    {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0)
            return;

        if (pid == -1)
        {
            if (errno == EINTR)
                continue;

            log_error("waitpid errno(%d): %s", errno, strerror(errno));
            return;
        }

        log_info("child-process pid(%d) exit status(%d)", pid, status);
    }
}
#endif

#ifdef __APPLE__
int daemon(int nochdir, int noclose)
{
    switch (fork())
    {
        case -1:
            return -1;
        case 0:
            break;
        default:
            _exit(0);
    }

    if (setsid() == -1)
        return -1;

    if (!nochdir)
        chdir("/");

    if (!noclose)
    {
        int fd = open("/dev/null", O_RDWR);
        if (fd == -1)
            return -1;

        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    return 0;
}
#endif

SystemManager::SystemManager(int argc, char *argv[]) : _config(argc, argv), _running_flag(true), _argc(argc), _argv(argv), _argv_max_len()
{
    system_manager = this;

#if !defined(_WIN32)
    init_set_proc_title();
#endif
    lua_port_init();

    try
    {
        lua_core_init(lua_state);
    }
    catch (const std::runtime_error &err)
    {
        log_error("%s", err.what());
        _exit(-1);
    }

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

    if (_config.env()->daemon)
    {
        log_info("enter daemon mode");

        int ret = daemon(1, 0);
        if (ret == -1)
            throw_system_error(errno, "daemon");
    }

    log_info("SystemManager(v%s) start running, pid=%d, daemon=%s", CORE_VERSION, getpid());
}

SystemManager::~SystemManager()
{
    lua_port_uninit();

    log_info("SystemManager exit, pid=%d", getpid());
}

void SystemManager::run()
{
    profile_start();

    _timer_mgr.set_sleep(std::bind(&SocketManager::wait_event, std::ref(_socket_mgr), std::placeholders::_1));
    while (_running_flag)
    {
        _timer_mgr.update();
    }

    profile_stop();
}

void SystemManager::set_proc_title(const char *title)
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

void SystemManager::on_timer(Timer *timer)
{
    lua_gc(lua_state, LUA_GCSTEP, 0);
    world->gc();
#ifdef _WIN32
    _socket_mgr.gc();
#endif
}

void SystemManager::on_fork(int pid)
{
    _log_ctx.on_fork(pid);
    _socket_mgr.on_fork(pid);
}

void SystemManager::profile_start()
{
#ifdef GPERFTOOLS
    const char *profile = config->get_string("profile", "./stmd.profile");

    ProfilerStart(profile);
    log_info("profile start %s", profile);
#endif
}

void SystemManager::profile_stop()
{
#ifdef GPERFTOOLS
    ProfilerStop();
    log_info("profile stop");
#endif
}

void SystemManager::start(LuaObject *init_object)
{
    set_timer(this, &SystemManager::on_timer, 100);
}

void SystemManager::stop() noexcept
{
}

void SystemManager::init_set_proc_title()
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

void SystemManager::lua_core_init(lua_State *L)
{
    config->copy_to_lua(L);

    const char *lua_path = config->get_string("lua_path");
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

    const char *start = config->get_string("start");
    logic_assert(start, "config.start is empty");

    int ret = luaL_loadfile(L, start);
    if (ret != LUA_OK)
        throw_lua_error(L);

    int top = lua_gettop(L);
    ret = lua_btcall(L, 0, 1);
    if (ret != LUA_OK)
        throw_lua_error(L);

    if (lua_istable(L, -1))
    {
        std::shared_ptr<LuaComponent> component(new LuaComponent());
        component->attach(L, -1);

        world->create_object_with_component(component);
    }
    lua_settop(L, top);
}

void SystemManager::lua_core_openlibs(lua_State *L)
{
    lua_class_define<Entity>(L);
    lua_class_define<Component>(L);
    lua_class_define<LuaComponent, Component>(L);

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
    lua_class_define<Messenger, Component>(L);

    lua_class_define<ProtoBase>(L);
    lua_class_define<LuapObject, ProtoBase>(L);
    lua_class_define<RespObject, ProtoBase>(L);
}
