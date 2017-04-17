CC     ?= gcc
XFLAGS ?= $(shell pkg-config vmguestlib --cflags --libs 2>/dev/null)

all: vmguest-statd

vmguest-statd: vmguest-statd.c
	$(CC) -o $@ $(XFLAGS) $<

restart: vmguest-statd
	-killall vmguest-statd
	./vmguest-statd >/dev/null

clean:
	-rm -f vmguest-statd *.o

.PHONY: clean
