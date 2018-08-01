#pragma once

#include <cstddef> // size_t
#include <string>
#include "lua_port.h"

void lux_proto_pack(std::string &str, lua_State *L, int index);
void lux_proto_pack_args(std::string &str, lua_State *L, int n);

size_t lux_proto_unpack(lua_State *L, const std::string &str, size_t pos);
int lux_proto_unpack_args(lua_State *L, const std::string &str);
