#$Id$

BIN = lx200.a 

all: $(BIN) 

include ../../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

lx200.a: lx200.o
	$(AR) rcs $@ $^
