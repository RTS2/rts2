# $Id$

bin_PROGRAMS = rts2-planc

noinst_HEADERS = selector.h

rts2_planc_SOURCES = planc.c selector.c

EXTRA_DIST = selector.ec

AM_CFLAGS = @LIBPG_CFLAGS@

rts2_planc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_PTHREAD@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2fits.a ../db/libdb.a ../utils/libdevcli.a

%.c: %.ec
	@ECPG@ -o $@ $^
