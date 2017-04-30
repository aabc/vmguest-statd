CC      ?= gcc
XFLAGS  ?= $(shell pkg-config vmguestlib --cflags --libs 2>/dev/null)
UNITDIR ?= /usr/lib/systemd/system

all: vmguest-statd

vmguest-statd: vmguest-statd.c
	$(CC) $< -o $@ $(XFLAGS)

restart: vmguest-statd
	-killall vmguest-statd
	./vmguest-statd >/dev/null

install: vmguest-statd
	install -p -D -m 0755 $< $(DESTDIR)/usr/sbin/$<
ifeq ($(wildcard $(UNITDIR)),)
	install -p -D -m 0755 vmguest-statd.sysvinit $(DESTDIR)/etc/init.d/vmguest-statd
 ifeq ($(DESTDIR),)
  ifeq ($(wildcard /sbin/chkconfig),/sbin/chkconfig)
	chkconfig --add vmguest-statd
  endif
  ifeq ($(wildcard /usr/sbin/update-rc.d),/usr/sbin/update-rc.d)
	update-rc.d vmguest-statd defaults
  endif
	service vmguest-statd start
 endif
else
	install -p -D -m 0644 vmguest-statd.service $(DESTDIR)$(UNITDIR)/vmguest-statd.service
 ifeq ($(DESTDIR),)
	systemctl daemon-reload
	systemctl enable --now vmguest-statd
 endif
endif

uninstall:
	-rm -f $(DESTDIR)/usr/sbin/vmguest-statd
ifeq ($(wildcard $(UNITDIR)),)
 ifeq ($(DESTDIR),)
	-service vmguest-statd stop
  ifeq ($(wildcard /sbin/chkconfig),/sbin/chkconfig)
	-chkconfig --del vmguest-statd
  endif
  ifeq ($(wildcard /usr/sbin/update-rc.d),/usr/sbin/update-rc.d)
	update-rc.d vmguest-statd remove
  endif
 endif
	-rm -f $(DESTDIR)/etc/init.d/vmguest-statd
else
 ifeq ($(DESTDIR),)
	-systemctl disable --now vmguest-statd
 endif
	-rm -f $(DESTDIR)$(UNITDIR)/vmguest-statd.service
 ifeq ($(DESTDIR),)
	-systemctl daemon-reload
 endif
endif

clean:
	-rm -f vmguest-statd *.o

.PHONY: install uninstall restart clean all
