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
AfterBuild = cp $(shell find . -name "*.h") ../include/lux

$(shell mkdir -p ../include/lux)

include ../utils/makefile

ifeq ($(UnixName),Linux)
Lib += rt anl
endif

ifeq ($(Config),Profile)
Lib += profiler tcmalloc
Define += GPERFTOOLS
endif
