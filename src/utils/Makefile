# $Id$

all: devdem.o devcli.o hms.o

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -l$(LIB_M)

test: hms.o test.o

