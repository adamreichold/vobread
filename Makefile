CFLAGS ?= -Wall -O2
LFLAGS ?= -ldvdread
PREFIX ?= /usr/local

vobread: vobread.c
	$(CC) $(CFLAGS) $(LFLAGS) vobread.c -o vobread

.PHONY: install
install: vobread
	install vobread $(DESTDIR)$(PREFIX)/bin/
