# $Id$
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread

all: test devdem.o

test: hms.o test.o

clean: 
	$(RM) *.o *~ 

mrproper:
	$(RM) *.o *~ test

indent:
	indent *.c *.h

