#pragma once

#define CORE_VERSION "0.1.0"

#include "component.h"

class Timer;
struct lua_State;

class LuxCore final : public Component
{
public:
    LuxCore(int argc, char *argv[]);
    ~LuxCore();

    static LuxCore * inst();

    void run();
    void set_proc_title(const char *title);
    void on_timer(Timer *timer);
    void on_fork(int pid);
    void profile_start();
    void profile_stop();

    virtual void start() override;
    virtual void stop() noexcept override;

    void quit() { _running_flag = false; }

private:
    void init_set_proc_title();
    void lua_core_init(lua_State *L);
    void lua_core_openlibs(lua_State *L);

    volatile bool _running_flag;
    int _argc;
    char **_argv;
    size_t _argv_max_len;
};
