#$Id$

BIN = camd_sbig camd_urvc

all: $(BIN) 

SUBDIRS = sbig urvc

include ../../make.conf

LDFLAGS += -l$(LIB_M) -l$(LIB_PTHREAD)

clean:
	$(RM) $(TOCLEAN) $(BIN)
	for x in $(SUBDIRS); do			\
		make -C $$x clean;		\
	done

sbig/sbig.a: camera.h sbig/Makefile
	$(MAKE) -C sbig

camd_sbig: camd.o sbig/camera.o sbig/sbig.o sbig/pardrv.aa ../utils/devdem.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

camd_urvc: camd.o urvc/marmicro.o urvc/mario.o urvc/marecp.o urvc/ccd_imaging.o urvc/ccd_tracking.o urvc/camera.o ../utils/devdem.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS) 
