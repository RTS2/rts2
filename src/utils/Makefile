# $Id$

all: devdem.o devcli.o hms.o devdemcli.a

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -l$(LIB_M)

devdemcli.a: devdem.o devcli.o hms.o
	$(AR) rcs devdemcli.a devdem.o devcli.o hms.o

test: hms.o test.o

