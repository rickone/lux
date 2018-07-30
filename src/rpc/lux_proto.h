#pragma once

#include "buffer.h"

size_t lux_proto_pack(Buffer *buffer, lua_State *L, int index);
size_t lux_proto_pack_args(Buffer *buffer, lua_State *L, int n);

size_t lux_proto_unpack(lua_State *L, Buffer *buffer, size_t len);
int lux_proto_unpack_args(lua_State *L, Buffer *buffer, size_t len);
