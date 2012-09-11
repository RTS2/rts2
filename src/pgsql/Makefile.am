# $Id$

instdir = @PG_CONFIG_SERVER_LIBS@

AM_CFLAGS = @LIBPG_SERVER_CFLAGS@ @LIBPG_CFLAGS@ -I../../include

if PGSQL

inst_LTLIBRARIES = pg_astrolib.la pg_wcs2.la

pg_astrolib_la_SOURCES = pg_astrolib.c
pg_astrolib_la_LDFLAGS = -module @LIB_NOVA@ @LIB_M@

pg_wcs2_la_SOURCES = pg_wcs2.c
pg_wcs2_la_LDFLAGS = -module @LIB_M@

if LIBWCS

inst_LTLIBRARIES += pg_wcs.la

pg_wcs_la_SOURCES = pg_wcs.c
pg_wcs_la_LDFLAGS = -module @LIB_M@ @LIBWCS_LDFLAGS@
pg_wcs_la_CFLAGS = ${AM_CFLAGS} @LIBWCS_CFLAGS@ 

AM_CFLAGS += @LIBWCS_CFLAGS@

endif

endif
