# $Id$

bin_PROGRAMS = rts2-bckimages rts2-flatprocess

nodist_rts2_bckimages_SOURCES = bckimages.c

EXTRA_DiST = bckimages.ec

rts2_flatprocess_SOURCES = flatprocess.c

AM_CFLAGS = @LIBPG_CFLAGS@ -I@top_srcdir@/include

rts2_bckimages_LDFLAGS = ../db/libdb.a ../utils/libdevcli.a @LDFLAGS@ @LIBPG_LIBS@ @LIB_ECPG@ 
rts2_flatprocess_LDFLAGS = ../utils/libdevcli.a @LIB_CFITSIO@ @LIB_M@

%.c: %.ec
	@ECPG@ -o $@ $^
