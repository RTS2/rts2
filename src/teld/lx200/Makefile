#$Id$

BIN = lx200.a 

all: $(BIN) 

CFLAGS += -g -I/home/petr/rts2/include/
LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD) 
lx200.a: lx200.o
	$(AR) rcs $@ $^
