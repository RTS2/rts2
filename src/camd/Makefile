#$Id$

BIN = camd

all: $(BIN) 

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

camd: camd.o sbig.o pardrv.a ../utils/devdem.o

