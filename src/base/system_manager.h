#pragma once

#define CORE_VERSION "0.1.0"

#include "component.h"
#include "config.h"
#include "log.h"
#include "timer_manager.h"
#include "socket_manager.h"

struct lua_State;

class SystemManager final : public Component
{
public:
    SystemManager(int argc, char *argv[]);
    ~SystemManager();

    void run();
    void set_proc_title(const char *title);
    void on_timer(Timer *timer);
    void on_fork(int pid);
    void profile_start();
    void profile_stop();

    virtual void start(LuaObject *init_object) override;
    virtual void stop() noexcept override;

    void quit() { _running_flag = false; }

private:
    void init_set_proc_title();
    void lua_core_init(lua_State *L);
    void lua_core_openlibs(lua_State *L);

    Config        _config;
    LogContext    _log_ctx;
    TimerManager  _timer_mgr;
    SocketManager _socket_mgr;

    volatile bool _running_flag;
    int _argc;
    char **_argv;
    size_t _argv_max_len;
};

extern SystemManager *system_manager;
