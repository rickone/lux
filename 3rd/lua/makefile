Target  = liblua.a
Include = src
Source	= $(shell ls src/*.c)
LibDir	=
Lib 	=
ObjDir	= ../../obj
OutDir	= ../../lib
Define 	= LUA_USE_POSIX
CFLAGS	=
CXXFLAGS =
LDFLAGS =
AfterBuild = cp src/lua.h src/lualib.h src/lauxlib.h src/luaconf.h src/lua.hpp ../../include/lua

$(shell mkdir -p ../../include/lua)

include ../../utils/makefile
