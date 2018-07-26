.PHONY: all clean luxd lux lua kcp

all: luxd

clean:
	rm -rf bin
	rm -rf lib
	rm -rf obj
	rm -rf include

luxd: lux
	$(MAKE) -C luxd

lux: lua kcp
	$(MAKE) -C src

lua:
	$(MAKE) -C 3rd/lua

kcp:
	$(MAKE) -C 3rd/kcp
