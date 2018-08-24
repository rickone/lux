#pragma once

#include "component.h"
#include "lua_port.h"

class LuaState final : public Component
{
public:
    LuaState() = default;
    ~LuaState();

    static std::shared_ptr<LuaState> create(int argc, char *argv[]);

    void init(int argc, char *argv[]);

    virtual void start() override;
    virtual void stop() noexcept override;

    lua_State *lua_state() { return _state; }

private:
    void lua_load_config(lua_State *L, const char *config_path);
    void lua_core_init(lua_State *L);
    void lua_core_openlibs(lua_State *L);

    lua_State *_state;
    Config _config;
};
