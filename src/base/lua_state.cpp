#include "lua_state.h"

LuaState::~LuaState()
{
    if (_state)
    {
        lua_close(_state);
        _state = nullptr;
    }
}

static std::shared_ptr<LuaState> LuaState::create(int argc, char *argv[])
{

}

void LuaState::init(int argc, char *argv[])
{
    logic_assert(argc >= 2, "argc = %d", argc);

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_core_openlibs(L);
    lua_load_config(L, argv[1]);
}

void LuaState::start()
{
}

void LuaState::stop() noexcept
{
}

void LuaState::lua_load_config(lua_State *L, const char *config_path)
{
    // call loadfile env=config

}

void LuaState::lua_core_init(lua_State *L)
{
    config->copy_to_lua(L);

    const char *lua_path = config->get_string("lua_path");
    if (lua_path)
    {
        std::string path(lua_path);
        if (!path.empty() && path.back() != ';')
            path.append(1, ';');

        int top = lua_gettop(L);
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        path.append(lua_tostring(L, -1));
        lua_pushlstring(L, path.data(), path.size());
        lua_setfield(L, -3, "path");
        lua_settop(L, top);
    }

    lua_core_openlibs(L);

    const char *start = config->get_string("start");
    logic_assert(start, "config.start is empty");

    int ret = luaL_loadfile(L, start);
    if (ret != LUA_OK)
        luaL_error(L, "loadfile(%s) error: %s", start, lua_tostring(L, -1));

    lua_call(L, 0, 1);
    world->start_lua_component(L);
}

void LuaState::lua_core_openlibs(lua_State *L)
{
    lua_class_define<Buffer>(L);
    lua_class_define<Config>(L);

    lua_class_define<Entity>(L);
    lua_class_define<Component>(L);
    lua_class_define<Timer>(L);
    lua_class_define<Socket, Component>(L);
    lua_class_define<TcpSocket, Socket>(L);
    lua_class_define<TcpSocketListener, Socket>(L);
    lua_class_define<UdpSocket, Socket>(L);
#if !defined(_WIN32)
    lua_class_define<UdpSocketListener, Socket>(L);
    lua_class_define<UnixSocket, Socket>(L);
    lua_class_define<UnixSocketStream, UnixSocket>(L);
    lua_class_define<UnixSocketListener, Socket>(L);
#endif
    lua_class_define<SocketKcp, Component>(L);
    lua_class_define<SocketPackage, Component>(L);

    lua_lib(L, "lux_core");
    {
        lua_set_function(L, "pack", luap_pack);
        lua_set_function(L, "unpack", luap_unpack);
        lua_std_function(L, log);
    }
    lua_pop(L, 1);
}
