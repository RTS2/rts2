# $Id$

all: devdem.a devser.a test

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -l$(LIB_M)

devdem.o: devser.c devser.h
	$(CC) -c -o devdem.o devser.c $(CFLAGS) -DDEVDEM_WITH_CLIENT

devdem.a: devdem.o devcli.o hms.o
	$(AR) rcs devdem.a devdem.o devcli.o hms.o

devser.a: devser.o devcli.o hms.o
	$(AR) rcs devser.a devser.o hms.o

test: hms.o test.o

