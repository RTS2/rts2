# $Id$
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread

all: devdem.o devcli.o hms.o

test: hms.o test.o

clean: 
	$(RM) *.o *~ 

mrproper:
	$(RM) *.o *~ test

indent:
	indent *.c *.h

