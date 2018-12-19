#include "log.h"
#include <cassert>
#include <cstdarg> // va_list
#include <sys/stat.h>
#include <time.h>
#if defined(_WIN32)
#include <codecvt>
#else
#include <syslog.h>
#endif
#include "config.h"
#include "error.h"
#include "lua.hpp"

using namespace lux;

void LogFile::set_file_path(const char *log_file_path)
{
    _log_file_path = log_file_path;

    time_t time_value = 0;
    struct stat file_stat;
    int ret = stat(log_file_path, &file_stat);
    if (ret == 0)
    {
        time_value = (time_t)file_stat.st_mtime;
    }
    else
    {
        time_value = time(0);
    }

    struct tm *tm_last_log = localtime(&time_value);
    if (tm_last_log == nullptr)
        throw_unix_error("localtime");

    change_log_file(tm_last_log);
}

void LogFile::write_line(const struct tm *tm_now, const std::string &log_text)
{
    if (_log_file_path == "")
        return;

    if (tm_now->tm_year != _tm_last_log.tm_year || tm_now->tm_mon != _tm_last_log.tm_mon || tm_now->tm_mday != _tm_last_log.tm_mday)
        change_log_file(tm_now);

    if (_ofs.is_open())
        _ofs << log_text << std::endl;
}

void LogFile::on_fork(int pid)
{
    _ofs.close();
    
    if (_log_file_path != "")
    {
        static char temp[32];
        snprintf(temp, sizeof(temp), ".%d", pid);

        std::string new_log_file_path = _log_file_path + std::string(temp);
        set_file_path(new_log_file_path.c_str());
    }
}

void LogFile::change_log_file(const struct tm *tm_last_log)
{
    if (_ofs.is_open())
    {
        _ofs.close();

        static char temp[32];
        snprintf(temp, sizeof(temp), ".%d_%02d_%02d", _tm_last_log.tm_year + 1900, _tm_last_log.tm_mon + 1, _tm_last_log.tm_mday);

        std::string last_file_path = _log_file_path + temp;

        int ret = rename(_log_file_path.c_str(), last_file_path.c_str());
        if (ret != 0)
            throw_unix_error("rename");
    }

    _ofs.open(_log_file_path, std::ofstream::out | std::ofstream::app);
    _tm_last_log = *tm_last_log;
}

LogContext::LogContext() : _log_mask(0xff)
{
}

LogContext::~LogContext()
{
#if !defined(_WIN32)
    if (_sys_log)
    {
        closelog();
        _sys_log = false;
    }
#endif
}

void LogContext::init()
{
#if !defined(_WIN32)
    const char *sys_log_ident = Config::inst()->get_string("sys_log");
    if (sys_log_ident)
    {
        _sys_log = true;
        openlog(sys_log_ident, LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
    }
#endif

    const char *local_log_file_path = Config::inst()->get_string("local_log");
    if (local_log_file_path)
    {
        _local_log_file.set_file_path(local_log_file_path);
    }

    const char *error_log_file_path = Config::inst()->get_string("error_log");
    if (error_log_file_path)
    {
        _error_log_file.set_file_path(error_log_file_path);
    }

    int log_level = Config::env()->log_level;
    if (log_level >= 0)
    {
#ifdef _WIN32
        set_log_mask((1 << (log_level + 1)) - 1);
#else
        set_log_mask(LOG_UPTO(log_level));
#endif
    }
}

LogContext *LogContext::inst()
{
    static LogContext s_inst;
    return &s_inst;
}

void LogContext::log(int level, const char *str)
{
    static const char *s_level_name[] = {
        "Emerg",
        "Alert",
        "Crit",
        "Err",
        "Warning",
        "Notice",
        "Info",
        "Debug",
    };
    static char s_head_buffer[32];

#if !defined(_WIN32)
    if (_sys_log)
        syslog(LOG_USER | level, "%s", str);
#endif

    if (!((1 << level) & _log_mask))
        return;

    time_t time_now = time(0);
    struct tm *tm_now = localtime(&time_now);
    if (tm_now == nullptr)
        throw_unix_error("localtime");

    int ret = snprintf(s_head_buffer, sizeof(s_head_buffer), "%02d:%02d:%02d [%s] ",
        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, s_level_name[level]);
    if (ret < 0)
        throw_unix_error("snprintf");

    std::string log_text(s_head_buffer, ret);

    log_text.append(str);

    int fd = STDOUT_FILENO;
    _local_log_file.write_line(tm_now, log_text);
    if (level <= kLevelError)
    {
        _error_log_file.write_line(tm_now, log_text);
        fd = STDERR_FILENO;
    }

    size_t len = strlen(str);

    write(fd, str, len);
    write(fd, "\n", 1);
}

void LogContext::log_format(int level, const char *fmt, ...)
{
    static char s_log_buffer[1024];

    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(s_log_buffer, sizeof(s_log_buffer), fmt, args);
    va_end(args);

    if (ret < 0)
        throw_unix_error("vsnprintf");

    log(level, s_log_buffer);
}

void LogContext::set_log_mask(int mask)
{
#if !defined(_WIN32)
    if (_sys_log)
        setlogmask(mask);
#endif

    _log_mask = mask;
}

void LogContext::on_fork(int pid)
{
    _local_log_file.on_fork(pid);
    _error_log_file.on_fork(pid);
}

int lua_log(lua_State *L)
{
    const char *str = luaL_checkstring(L, 1);
    LogContext::inst()->log(kLevelInfo, str);
    return 0;
}
