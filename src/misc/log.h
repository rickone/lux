#pragma once

#include <string>
#include <fstream>
#include "component.h"

enum LogLevel
{
    kLevelEmerg,
    kLevelAlert,
    kLevelCrit,
    kLevelError,
    kLevelWarning,
    kLevelNotice,
    kLevelInfo,
    kLevelDebug,
};

class LogFile final
{
public:
    LogFile() = default;
    ~LogFile() = default;

    void set_file_path(const char *log_file_path);
    void write(const struct tm *tm_now, const std::string &log_text);
    void on_fork(int pid);
    
private:
    void change_log_file(const struct tm *tm_last_log);

    std::string _log_file_path;
    struct tm _tm_last_log;
    std::ofstream _ofs;
};

class LogContext final : public Component
{
public:
    LogContext();
    virtual ~LogContext();

    static LogContext * inst();

    void init();
    void log(int level, const char *fmt, ...);
    void set_log_mask(int mask);

    void on_fork(int pid);

private:
    bool _sys_log;
    int _log_mask;

    LogFile _local_log_file;
    LogFile _error_log_file;
};

#define log_line(Level, Fmt, ...) \
    do \
    { \
        LogContext *inst = LogContext::inst(); \
        if (inst) \
            inst->log(Level, Fmt,## __VA_ARGS__); \
    } while (false)

#define log_debug(Fmt, ...) log_line(kLevelDebug, Fmt,## __VA_ARGS__)
#define log_info(Fmt, ...)  log_line(kLevelInfo, Fmt,## __VA_ARGS__)
#define log_error(Fmt, ...) log_line(kLevelError, Fmt,## __VA_ARGS__)

extern int lua_log(lua_State *L);
