#$Id$

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

all: camd

camd: camd.o sbig.o pardrv.a ../utils/devdem.o

clean: 
	$(RM) *.o *~ core

mrproper:
	$(RM) *.o *~ core camd

indent:
	indent *.c *h
