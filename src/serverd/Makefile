# $Id$

BIN = serverd

all: $(BIN)

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

serverd: serverd.o ../utils/devser.a
