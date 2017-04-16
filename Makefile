CC     ?= gcc
XFLAGS ?= $(shell pkg-config vmguestlib --cflags --libs 2>/dev/null)

all: vmguest-statd

vmguest-statd: vmguest-statd.c
	$(CC) -o $@ $(XFLAGS) $<

clean:
	-rm -f vmguest-statd *.o

.PHONY: clean
