#include "lux_core.h"
#include <algorithm> // std::min
#include <signal.h>
#include "config.h"
#include "log.h"
#include "object_manager.h"
#include "timer_manager.h"
#include "socket_manager.h"
#include "lua_state.h"
#include "task_manager.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <process.h>
#define getpid _getpid

#ifdef min
#undef min
#endif

#else
#include <unistd.h> // getpid, fork
#include <sys/wait.h>

extern char **environ;
#endif

#ifdef __linux__
#include <sys/prctl.h>
#endif

#ifdef GPERFTOOLS
#include <gperftools/profiler.h>
#endif

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

LuxCore * LuxCore::inst()
{
    static std::shared_ptr<LuxCore> s_inst(new LuxCore());
    return s_inst.get();
}

int LuxCore::main(int argc, char *argv[])
{
    auto core = LuxCore::inst();
    core->init(argc, argv);
    core->run();
    return 0;
}

void LuxCore::init(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);

    CONSOLE_FONT_INFOEX cfi = { 0 };
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = DEVICE_DEFAULT_FONT;
    cfi.dwFontSize.Y = 20;
    cfi.FontFamily = TMPF_VECTOR | TMPF_TRUETYPE | FF_MODERN;
    cfi.FontWeight = FW_NORMAL;
    wcscpy(cfi.FaceName, L"Consolas");

    HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetCurrentConsoleFontEx(hstdout, FALSE, &cfi);

#ifdef _DEBUG
    std::string title("lux " CORE_VERSION " [Debug]");
#else
    std::string title("lux " CORE_VERSION " [Release]");
#endif
    SetConsoleTitle(title.c_str());
#else
    init_set_proc_title(argc, argv);
#endif

    Config::inst()->init(argc, argv);
    LogContext::inst()->init();
    TimerManager::inst()->init();
    SocketManager::inst()->init();
    LuaState::inst()->init();
    TaskManager::inst()->init();

    auto timer = Timer::create(200);
    timer->on_timer.set(this, &LuxCore::on_gc);

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

#if !defined(_WIN32)
    if (Config::env()->daemon)
    {
        log_info("enter daemon mode");

        int ret = daemon(1, 0);
        if (ret == -1)
            throw_unix_error("daemon");
    }
#endif

    log_info("lux start (pid=%d, version=%s, build-time='%s %s')", getpid(), CORE_VERSION, __TIME__, __DATE__);
}

void LuxCore::run()
{
    profile_start();

    auto timer_mgr = TimerManager::inst();
    auto socket_mgr = SocketManager::inst();

    _running_flag = true;
    while (_running_flag)
    {
        int notick_time = timer_mgr->tick();
        socket_mgr->wait_event(notick_time);
    }

    profile_stop();

    log_info("lux exit (pid=%d)", getpid());
}

void LuxCore::set_proc_title(const char *title)
{
#ifndef _WIN32
    size_t title_len = strlen(title) + 1;
    size_t copy_len = std::min(title_len, _argv_max_len);

    memcpy(_argv[0], title, copy_len);
    _argv[0][copy_len - 1] = '\0';

    _argv[1] = nullptr;
    memset(_argv[0] + copy_len, 0, _argv_max_len - copy_len);

#ifdef __linux__
    prctl(PR_SET_NAME, title);
#endif

#endif
}

void LuxCore::on_gc()
{
    LuaState::inst()->gc();
    ObjectManager::inst()->gc();
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

void LuxCore::init_set_proc_title(int argc, char *argv[])
{
    _argc = argc;
    _argv = argv;

#ifndef _WIN32
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
#endif
}
