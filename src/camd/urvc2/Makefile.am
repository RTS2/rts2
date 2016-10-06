noinst_LIBRARIES = liburvc2.a
noinst_HEADERS = urvc.h 

if SUNCYGMAC
EXTRA_DIST = io.c micro.c st237.c trk.c ccd.c sexif.c st7.c tools.c
else
if ARMCPU
EXTRA_DIST = io.c micro.c st237.c trk.c ccd.c sexif.c st7.c tools.c
else
liburvc2_a_SOURCES = io.c micro.c st237.c trk.c ccd.c sexif.c st7.c tools.c
liburvc2_a_CFLAGS = -O3 ${CFITSIO_CFLAGS} -I$(READLINE) -Wall -Wshadow -g -I../../../include #-DDEBUG
endif
endif
