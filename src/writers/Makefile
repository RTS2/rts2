# $Id$

all: fits.o

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -L/usr/src/cfitsio -lcfitsio -l$(LIB_M)

fits.o: 
	$(CC) -c -o fits.o fits.c $(CFLAGS)

