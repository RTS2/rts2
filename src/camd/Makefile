#$Id$

BIN = camd_sbig

all: $(BIN) 

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

sbig/sbig.a: camera.h sbig/Makefile
	$(MAKE) -C sbig

camd_sbig: camd.o sbig/sbig.a ../utils/devdem.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)
