# $Id$

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

all: teld

clean: 
	$(RM) *.o *~ core

mrproper:
	$(RM) *.o *~ core teld

indent:
	indent *.h *.c

teld: teld.o lx200.o ../utils/hms.o ../utils/devdem.o

commit: indent clean
	cvs commit
