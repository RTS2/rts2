# $Id$

bin_PROGRAMS = rts2-fits2db rts2-f2cj

bin_SCRIPTS = rts2-findimages

noinst_HEADERS = logo.h

rts2_fits2db_SOURCES = fits2db.c
rts2_fits2db_LDFLAGS = @LIB_CFITSIO@ ../utils/config.o

rts2_f2cj_SOURCES = f2cj.c
rts2_f2cj_LDFLAGS = @LIB_WCS@ @LIB_CFITSIO@ @LIB_JPEG@
