#include "config.h"
#include <fstream> // ifstream
#include <memory>
#ifdef _WIN32
#include "getopt_win32.h"
#else
#include <getopt.h> // getopt_long
#endif
#include "lua.hpp"

Config * Config::inst()
{
    static Config s_inst;
    return &s_inst;
}

ConfigEnv * Config::env()
{
    return &(inst()->_env);
}

void Config::init(int argc, char *argv[])
{
    static char short_options[] = "";
    static struct option long_options[] = {
       {"config", required_argument, 0, 0},
       {"start", required_argument, 0, 0},
       {"daemon", required_argument, 0, 0},
       {"sys_log", required_argument, 0, 0},
       {"local_log", required_argument, 0, 0},
       {"error_log", required_argument, 0, 0},
       {"profile", required_argument, 0, 0},
       {"extra", required_argument, 0, 0},

       {0, 0, 0, 0}
    };

    for (;;)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c < 0)
            break;

        if (c == 0)
        {
            const char *value = "true";
            if (optarg)
                value = optarg;

            set_field(long_options[option_index].name, value);
        }
    }

    const char *conf_path = get_string("config");
    if (conf_path)
        load_conf(conf_path);

    load_env();
    dump();
}

void Config::load_conf(const char *conf_path)
{
    std::ifstream ifs(conf_path, std::ifstream::in);
    std::filebuf *pbuf = ifs.rdbuf();

    std::size_t file_size = pbuf->pubseekoff(0, ifs.end, ifs.in);
    pbuf->pubseekpos(0, ifs.in);

    char *buffer = new char[file_size];
    std::unique_ptr<char, std::default_delete<char[]> > uptr(buffer);
    pbuf->sgetn(buffer, file_size);

    ifs.close();

    std::string key;
    std::string value;
    bool blind = false;
    for (size_t i = 0; i < file_size; ++i)
    {
        char c = buffer[i];

        if (blind)
        {
            if (c == '\n')
                blind = false;

            continue;
        }

        switch (c)
        {
            case '#':
                blind = true;
            case '\n':
                if (key != "")
                {
                    value.erase(value.find_last_not_of(" ") + 1);
                    value.erase(0, value.find_first_not_of(" "));

                    auto it = m_dict.find(key);
                    if (it == m_dict.end())
                        set_field(key, value);
                }

                key = "";
                value = "";
                break;

            case '\r':
                break;

            case ' ':
                if (key == "")
                {
                    if (value != "")
                    {
                        key = value;
                        value = "";
                    }
                }
                else
                {
                    value += c;
                }
                break;

            default:
                value += c;
                break;
        }
    }
}

void Config::load_env()
{
    _env.daemon = get_boolean("daemon", false);
    _env.log_level = get_int("log_level", -1);
    _env.listen_backlog = get_int("listen_backlog", 1024);
    _env.socket_recv_buffer_init = get_size("socket_recv_buffer_init", 2 * 1024);
    _env.socket_send_buffer_init = get_size("socket_send_buffer_init", 2 * 1024);
    _env.socket_send_buffer_max = get_size("socket_send_buffer_max", 128 * 1024);
}

void Config::set_field(const std::string &field, const std::string &value)
{
    m_dict[field] = value;
}

void Config::set_field(const char *field, const char *value)
{
    set_field(std::string(field), std::string(value));
}

const char * Config::get_string(const char *field) const
{
    return get_string(field, nullptr);
}

const char * Config::get_string(const char *field, const char *def_value) const
{
    auto it = m_dict.find(field);
    if (it == m_dict.end())
        return def_value;

    return it->second.c_str();
}

void Config::dump() const
{
    puts("config = {");
    for (auto pair : m_dict)
    {
        printf("    %s = '%s',\n", pair.first.c_str(), pair.second.c_str());
    }
    puts("}");
}

void Config::copy_to_lua(lua_State *L)
{
    lua_newtable(L);
    for (auto pair : m_dict)
    {
        lua_pushlstring(L, pair.first.data(), pair.first.size());
        lua_pushlstring(L, pair.second.data(), pair.second.size());
        lua_settable(L, -3);
    }
    lua_setglobal(L, "config");
}

int Config::get_int(const char *field) const
{
    return get_int(field, 0);
}

int Config::get_int(const char *field, int def_value) const
{
    auto it = m_dict.find(field);
    if (it == m_dict.end())
        return def_value;

    return std::stoi(it->second);
}

size_t Config::get_size(const char *field) const
{
    return get_size(field, 0);
}

size_t Config::get_size(const char *field, size_t def_value) const
{
    auto it = m_dict.find(field);
    if (it == m_dict.end())
        return def_value;

    return (size_t)std::stoul(it->second);
}

bool Config::get_boolean(const char *field) const
{
    return get_boolean(field, false);
}

bool Config::get_boolean(const char *field, bool def_value) const
{
    auto it = m_dict.find(field);
    if (it == m_dict.end())
        return def_value;

    const std::string &str = it->second;
    return str == "true" || str == "on" || str == "ok";
}
