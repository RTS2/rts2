#$Id$
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm -lpthread -Wall
all: camd

camd: camd.o sbig.o pardrv.a ../utils/devdem.o

clean: 
	$(RM) -rf *.o *~ core

mrproper:
	$(RM) -rf *.o *~ core

indent:
	indent *.c *h
