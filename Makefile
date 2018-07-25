.PHONY: all clean stmd stm lua kcp

all: stmd

clean:
	rm -rf bin
	rm -rf lib
	rm -rf obj
	rm -f `find include -name "*.h"`

stmd: stm
	$(MAKE) -C stmd

stm: lua kcp
	$(MAKE) -C src

lua:
	$(MAKE) -C 3rd/lua

kcp:
	$(MAKE) -C 3rd/kcp
