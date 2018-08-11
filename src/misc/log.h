#pragma once

#include <string>
#include <fstream>

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

class LogContext final
{
public:
    LogContext();
    ~LogContext();

    void log(int level, const char *fmt, ...);
    void set_log_mask(int mask);

    void on_fork(int pid);

private:
    bool _sys_log;
    int _log_mask;

    LogFile _local_log_file;
    LogFile _error_log_file;
};

extern LogContext *log_context;

#define log_debug(Fmt, ...) if (log_context) log_context->log(kLevelDebug, Fmt,## __VA_ARGS__)
#define log_info(Fmt, ...)  if (log_context) log_context->log(kLevelInfo, Fmt,## __VA_ARGS__)
#define log_error(Fmt, ...) if (log_context) log_context->log(kLevelError, Fmt,## __VA_ARGS__)

struct lua_State;
extern int lua_log(lua_State *L);
