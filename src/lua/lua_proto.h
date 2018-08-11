#pragma once

#include <cstddef> // size_t
#include <string>
#include "lua.hpp"

/* Header Byte Code
 *
 * Varient Int:
 * 0xxx,xxxx -- 1B
 * 10xx,xxxx -> [1xxx,xxxx] -> [0xxx,xxxx] -- nB littile-endian
 * 1100,0000 -- 1B type
 *
 * 0xC0 - nil
 * 0xC1 - false
 * 0xC2 - true
 * 0xC3 - number
 * 0xC4 - string
 * 0xC5 - table
 * 0xC6 - args
 */

void lua_proto_pack(std::string &str, lua_State *L, int index);
void lua_proto_pack_args(std::string &str, lua_State *L, int n);

size_t lua_proto_unpack(lua_State *L, const std::string &str, size_t pos);
int lua_proto_unpack_args(lua_State *L, const std::string &str);

int luap_pack(lua_State *L);
int luap_unpack(lua_State *L);
