#pragma once

#include "error.h"
#include "lua_port.h"

class Buffer;

class ProtoBase : public LuaObject
{
public:
    ProtoBase() = default;
    virtual ~ProtoBase() = default;

    static void new_class(lua_State *L);
    
    virtual void serialize(Buffer *buffer);
    virtual bool deserialize(Buffer *buffer);
    virtual int pack(lua_State *L);
    virtual int unpack(lua_State *L);
};

class ProtoError : public std::runtime_error
{
public:
    explicit ProtoError(const std::string &what);
};
