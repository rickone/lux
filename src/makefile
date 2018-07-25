Target  = liblux.so
Include = ../include/lua ../include/kcp $(shell find . -type d)
Source  = $(shell find . -name "*.cpp")
LibDir  = ../lib
Lib     = lua kcp
ObjDir  = ../obj
OutDir  = ../lib
Define  =
CFLAGS  =
CXXFLAGS =
LDFLAGS =
AfterBuild = mkdir -p ../include/stm & cp $(shell find . -name "*.h") ../include/stm

include ../utils/makefile
