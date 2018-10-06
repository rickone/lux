#include "lua_state.h"
#include "config.h"
#include "tcp_socket.h"
#include "tcp_socket_listener.h"
#include "udp_socket.h"
#include "udp_socket_listener.h"
#include "unix_socket.h"
#include "unix_socket_stream.h"
#include "unix_socket_listener.h"
#include "socket_kcp.h"
#include "socket_package.h"
#include "lux_proto.h"

LuaState::~LuaState()
{
    if (_state != nullptr)
    {
        lua_close(_state);
        _state = nullptr;
        set_lua_state(nullptr);
    }
}

LuaState * LuaState::inst()
{
    static LuaState s_inst;
    return &s_inst;
}

void LuaState::init()
{
    lua_State *L = luaL_newstate();
    _state = L;
    set_lua_state(L);

    luaL_openlibs(L);
    lua_path_init(L);
    lua_core_openlibs(L);
    lua_start_run(L);
}

void LuaState::gc()
{
    lua_gc(_state, LUA_GCSTEP, 0);
}

void LuaState::lua_path_init(lua_State *L)
{
    const char *lua_path = Config::inst()->get_string("lua_path");
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
}

void LuaState::lua_core_openlibs(lua_State *L)
{
    lua_class_define<Timer>(L);
    lua_class_define<Buffer>(L);
    lua_class_define<Socket>(L);
    lua_class_define<TcpSocket, Socket>(L);
    lua_class_define<TcpSocketListener, Socket>(L);
    lua_class_define<UdpSocket, Socket>(L);
#if !defined(_WIN32)
    lua_class_define<UdpSocketListener, Socket>(L);
    lua_class_define<UnixSocket, Socket>(L);
    lua_class_define<UnixSocketStream, UnixSocket>(L);
    lua_class_define<UnixSocketListener, Socket>(L);
#endif
    lua_class_define<SocketKcp>(L);
    lua_class_define<SocketPackage>(L);
    lua_class_define<LuxProto>(L);

    lua_lib(L, "lux_core");
    {
        lua_std_function(L, log);
    }
    lua_pop(L, 1);
}

void LuaState::lua_start_run(lua_State *L)
{
    Config::inst()->copy_to_lua(L);

    const char *start = Config::inst()->get_string("start");
    logic_assert(start, "config.start is empty");

    int ret = luaL_loadfile(L, start);
    if (ret != LUA_OK)
        luaL_error(L, "loadfile(%s) error: %s", start, lua_tostring(L, -1));

    lua_call(L, 0, 0);
}
