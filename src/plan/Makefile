#$Id$

BIN = planc

all: $(BIN)

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio 
LDFLAGS += -L/usr/src/cfitsio -lcfitsio -l$(LIB_M) -l$(LIB_PTHREAD)

plan: plancom.o plan.o ../utils/devcli.o ../writers/fits.o
	$(CC) -o plan plan.o plancom.o ../utils/devcli.o ../writers/fits.o $(CFLAGS) $(LDFLAGS)

plancom.o: plancom.c ../writers/fits.o

planc: planc.o selector.o ../writers/fits.o ../utils/devcli.a
	$(CC) -o $@ $^ -lcfitsio -lm -lpthread -lecpg -lpq -lnova

selector.o: selector.ec
	ecpg selector.ec
	$(CC) -I/usr/include/postgresql selector.c -c -o selector.o -DDEBUG
