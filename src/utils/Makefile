# $Id$

all: devdem.a devser.a test

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -l$(LIB_M)

devdem.a: devdem.o devser.o devcli.o hms.o param.o
	$(AR) rcs devdem.a devser.o devdem.o devcli.o hms.o param.o

devser.a: devser.o hms.o param.o
	$(AR) rcs devser.a devser.o hms.o param.o

test: hms.o test.o
