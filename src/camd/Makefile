#$Id$
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm -lpthread -Wall
all: camd

camd: camd.o sbig.o pardrv.a ../utils/devdem.o

clean: 
	$(RM) *.o *~ core

mrproper:
	$(RM) *.o *~ core camd

indent:
	indent *.c *h

commit: indent clean
	cvs commit
