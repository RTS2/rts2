# $Id$

BIN = serverd test

all: $(BIN)

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD) -lnova
CFLAGS += -I/usr/local/include

serverd: serverd.o riseset.o ../utils/devser.a

riseset.o:riseset.c
	$(CC) $(CFLAGS) -c -o $@ $^ -lnova

test: riseset.o
