noinst_HEADERS = logo.h bootes.h

rts2_findimagesdir = ${datadir}/doc/@PACKAGE@/scripts

bin_SCRIPTS = rts2-findimages

dist_rts2_findimages_DATA = rts2-findimages

if LIBWCS
if GRAPHICSMAGICK

bin_PROGRAMS = rts2-f2cj

rts2_f2cj_SOURCES = f2cj.c
rts2_f2cj_LDFLAGS = @LIBWCS_LDFLAGS@ @LIB_CFITSIO@ @LIB_JPEG@
rts2_f2cj_CFLAGS = @LIBWCS_CFLAGS@

else
EXTRA_DIST = f2cj.c
endif
else
EXTRA_DIST = f2cj.c
endif
