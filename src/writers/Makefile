# $Id$

all: fits.o

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio

fits.o: fits.c fits.h 
	$(CC) -c -o fits.o fits.c $(CFLAGS)

