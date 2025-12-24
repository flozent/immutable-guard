CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDFLAGS = -lssl -lcrypto -lsystemd
PREFIX = /usr

all: immutable-guard immutable-guard-helper

immutable-guard: src/immutable-container-guard.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

immutable-guard-helper: src/immutable-guard-helper.c
	$(CC) $(CFLAGS) -o $@ $<

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/libexec
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/usr/lib/systemd/system
	install -d $(DESTDIR)/var/lib/immutable-guard/hashes
	
	install -m 755 immutable-guard $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 immutable-guard-helper $(DESTDIR)$(PREFIX)/libexec/
	install -m 644 config/immutable-guard.conf $(DESTDIR)/etc/
	install -m 644 config/immutable-guard.service $(DESTDIR)/usr/lib/systemd/system/
	
	chown -R user-12-32:user-12-32 $(DESTDIR)/var/lib/immutable-guard

clean:
	rm -f immutable-guard immutable-guard-helper

.PHONY: all install clean
