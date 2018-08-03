#pragma once

#include <cstddef> // size_t
#include <string>
#include "lua.hpp"

void lua_proto_pack(std::string &str, lua_State *L, int index);
void lua_proto_pack_args(std::string &str, lua_State *L, int n);

size_t lua_proto_unpack(lua_State *L, const std::string &str, size_t pos);
int lua_proto_unpack_args(lua_State *L, const std::string &str);

int luap_pack(lua_State *L);
int luap_unpack(lua_State *L);
