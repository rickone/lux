#pragma once

#include <string>
#include <unordered_map>

struct lua_State;

struct ConfigEnv
{
    bool            daemon;
    int             log_level;
    int             listen_backlog;
    size_t          socket_recv_buffer_init;
    size_t          socket_send_buffer_init;
    size_t          socket_send_buffer_max;
    int             thread_num;
    unsigned int    tick_interval;
};

class Config final
{
public:
    Config() = default;
    virtual ~Config() = default;

    static Config * inst();
    static ConfigEnv * env();

    void init(int argc, char *argv[]);
    void load_conf(const char *conf_path);
    void load_env();

    void set_field(const std::string &field, const std::string &value);
    void set_field(const char *field, const char *value);

    const char * get_string(const char *field) const;
    const char * get_string(const char *field, const char *def_value) const;

    void dump() const;
    void copy_to_lua(lua_State *L);

private:
     int get_int(const char *field, int def_value) const;
     unsigned int get_uint(const char *field, unsigned int def_value) const;
     bool get_boolean(const char *field) const;
     bool get_boolean(const char *field, bool def_value) const;

private:
    std::unordered_map<std::string, std::string> m_dict;
    ConfigEnv _env;
};
