#include "proto_base.h"
#include "buffer.h"

void ProtoBase::new_class(lua_State *L)
{
    lua_new_class(L, ProtoBase);

    lua_newtable(L);
    {
        lua_method(L, serialize);
        lua_method(L, deserialize);
        lua_method(L, pack);
        lua_method(L, unpack);
    }
    lua_setfield(L, -2, "__method");
}

void ProtoBase::serialize(Buffer *buffer)
{
}

bool ProtoBase::deserialize(Buffer *buffer)
{
    return false;
}

int ProtoBase::pack(lua_State *L)
{
    return 0;
}

int ProtoBase::unpack(lua_State *L)
{
    return 0;
}

ProtoError::ProtoError(const std::string &what) : std::runtime_error(what)
{
}
