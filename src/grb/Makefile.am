# $Id$

SUBDIRS = ibas bacodine

bin_PROGRAMS = rts2-grbc

noinst_HEADERS = grbc.h

rts2_grbc_SOURCES = grbc.cpp
rts2_grbc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_PTHREAD@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2fits.a ../db/libdb.a ../utils/libdevcli.a bacodine/libbacodine.a ibas/libibas.a
