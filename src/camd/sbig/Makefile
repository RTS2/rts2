#$Id$

BIN = sbig.a 

all: $(BIN) 

CFLAGS = -../../../include
LDFLAGS += -lm -lpthread

sbig.a: camera.o sbig.o pardrv.aa
	@cp@ -f pardrv.aa sbig.a
	$(AR) rcs sbig.a camera.o sbig.o 

