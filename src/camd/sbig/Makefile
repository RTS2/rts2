#$Id$

BIN = sbig.a 

all: $(BIN) 

include ../../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

sbig.a: camera.o sbig.o pardrv.aa
	cp -f pardrv.aa sbig.a
	$(AR) rcs sbig.a camera.o sbig.o 

