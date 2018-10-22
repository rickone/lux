#pragma once

#define CORE_VERSION "0.2.0"
#include "lua_port.h"

namespace lux {

class Core final : public Object
{
public:
    Core() = default;
    virtual ~Core() = default;

    static Core *inst();
    static int main(int argc, char *argv[]);

    void init(int argc, char *argv[]);
    void run();
    void set_proc_title(const char *title);
    void on_gc();
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

} // lux
