# $Id$

bin_PROGRAMS = rts2-fits2db rts2-wcs2db rts2-f2cj

noinst_HEADERS = logo.h

rts2_wcs2db_SOURCES = wcs2db.c
rts2_wcs2db_LDFLAGS = @LIB_WCS@

rts2_fits2db_SOURCES = fits2db.c
rts2_fits2db_LDFLAGS = @LIB_CFITSIO@

rts2_f2cj_SOURCES = f2cj.c
rts2_f2cj_LDFLAGS = @LIB_WCS@ @LIB_CFITSIO@ @LIB_JPEG@
