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

CFLAGS+= ${S_CFLAGS}
LDFLAGS+= ${S_LDFLAGS}

.PHONY: all clean

all:
	@echo "Found ${OS}"
	$(CC) $(CFLAGS) pcbcrd.c -o pcbcrd

clean:
	@rm -v pcbcrd



