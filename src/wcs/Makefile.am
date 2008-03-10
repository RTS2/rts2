# $Id$

if LIBWCS

rts2_findimagesdir = ${datadir}/doc/@PACKAGE@/scripts

bin_PROGRAMS = rts2-f2cj

bin_SCRIPTS = rts2-findimages

dist_rts2_findimages_DATA = rts2-findimages

noinst_HEADERS = logo.h bootes.h

rts2_f2cj_SOURCES = f2cj.c
rts2_f2cj_LDFLAGS = @LIBWCS_LDFLAGS@ @LIB_CFITSIO@ @LIB_JPEG@
rts2_f2cj_CFLAGS = @LIBWCS_CFLAGS@

endif
