# $Id$

BIN=test

all: devdem.a devser.a devcli.a test $(BIN)

include ../../make.conf

CFLAGS += -I/usr/src/cfitsio
LDFLAGS += -l$(LIB_M)

devdem.a: devdem.o devser.o devcli.o devhnd.o hms.o param.o
	$(AR) rcsv $@ $^

devser.a: devser.o hms.o param.o
	$(AR) rcsv $@ $^

devcli.a: devcli.o devhnd.o param.o hms.o
	$(AR) rcsv $@ $^

test: hms.o test.o
