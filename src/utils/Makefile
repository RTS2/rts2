# $Id$

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -l$(LIB_M)

all: devdem.o devcli.o hms.o

test: hms.o test.o

clean: 
	$(RM) *.o *~ 

mrproper:
	$(RM) *.o *~ test

indent:
	indent *.c *.h

commit:indent clean
	$(CVS) commit
