#pragma once

#include <string>
#include <unordered_map>

struct lua_State;

struct ConfigEnv
{
    int     log_level;
    int     listen_backlog;
    size_t  socket_recv_buffer_init;
    size_t  socket_send_buffer_init;
};

class Config final
{
public:
    Config(int argc, char *argv[]);
    ~Config() = default;

    void load_conf(const char *conf_path);
    void load_env();

    void set_field(const std::string &field, const std::string &value);
    void set_field(const char *field, const char *value);

    const char * get_string(const char *field) const;
    const char * get_string(const char *field, const char *def_value) const;

    void dump() const;
    void copy_to_lua(lua_State *L);

    const ConfigEnv * env() const { return &_env; }

private:
    int get_int(const char *field) const;
    int get_int(const char *field, int def_value) const;
    size_t get_size(const char *field) const;
    size_t get_size(const char *field, size_t def_value) const;
    bool get_boolean(const char *field) const;
    bool get_boolean(const char *field, bool def_value) const;

private:
    std::unordered_map<std::string, std::string> m_dict;
    ConfigEnv _env;
};

extern Config *config;
