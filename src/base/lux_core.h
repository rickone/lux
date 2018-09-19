#pragma once

#define CORE_VERSION "0.2.0"
#include "lua_port.h"

class Timer;

class LuxCore final : public LuaObject
{
public:
    LuxCore() = default;
    virtual ~LuxCore() = default;

    static LuxCore * inst();

    void init(int argc, char *argv[]);
    void run();
    void set_proc_title(const char *title);
    void on_gc(Timer *timer);
    void on_fork(int pid);
    void profile_start();
    void profile_stop();

    void quit() { _running_flag = false; }

private:
    void init_set_proc_title(int argc, char *argv[]);

    volatile bool _running_flag;
    int _argc;
    char **_argv;
    size_t _argv_max_len;
};
