# $Id$

BIN = teld

all: $(BIN)

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

teld: teld.o lx200.o ../utils/hms.o ../utils/devdemcli.a
