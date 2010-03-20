# Adapt paths for OSes
OS=$(shell uname -s)
ifeq (${OS}, Darwin)
    S_CFLAGS=-I/opt/local/include
    S_LDFLAGS=-L/opt/local/lib
else ifeq (${OS}, FreeBSD)
    S_CFLAGS=-I/usr/local/include -I/usr/include
    S_LDFLAGS=-L/usr/local/lib -L/usr/lib
else ifeq (${OS}, NetBSD)
    S_CFLAGS=-I/usr/pkg/include -I/usr/include
    S_LDFLAGS=-L/usr/pkg/lib -L/usr/lib
else ifeq (${OS}, OpenBSD)
    S_CFLAGS=-I/usr/local/include -I/usr/include
    S_LDFLAGS=-L/usr/local/lib -L/usr/lib
else
    S_CFLAGS=-I/usr/include
    S_LDFLAGS=-L/usr/lib
endif

LINKS=-lpthread -lxdo
CFLAGS+= ${S_CFLAGS} -std=c99 -g -ggdb3 -fno-inline
LDFLAGS+= ${S_LDFLAGS} $(LINKS)

.PHONY: all clean

all:
	@echo "Found ${OS}"
	$(CC) $(CFLAGS) $(LDFLAGS) $(LINKS) pcbcrd.c -o pcbcrd

clean:
	@rm -v pcbcrd



