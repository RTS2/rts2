# $Id$
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm -Wall
all: lx200.o teld

clean: 
	$(RM) -rf *.o *~ teld core

indent:
	indent *.h *.c

teld: teld.o lx200.o ../utils/hms.o ../utils/devdem.o

commit: indent clean
	cvs commit
