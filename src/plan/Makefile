#$Id$

all: plan

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -L/usr/src/cfitsio -lcfitsio -l$(LIB_M) -l$(LIB_PTHREAD)

plan: plancom.o plan.o ../utils/devcli.o ../writers/fits.o
	$(CC) -o plan plan.o plancom.o ../utils/devcli.o ../writers/fits.o $(CFLAGS) $(LDFLAGS)

plancom.o: plancom.c ../writers/fits.o

