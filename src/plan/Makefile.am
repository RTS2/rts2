# $Id$

bin_PROGRAMS = rts2-planc

rts2_planc_SOURCES = planc.c selector.c
rts2_planc_CFLAGS = @LIBPG_CFLAGS@
rts2_planc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_PTHREAD@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2fits.a ../db/libdb.a ../utils/libdevcli.a

%.c: %.ec
	@ECPG@ -o $@ $^
