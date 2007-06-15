# $Id$

instdir = @PG_CONFIG_SERVER_LIBS@

AM_CFLAGS = @LIBPG_SERVER_CFLAGS@ @LIBPG_CFLAGS@

if PGSQL

inst_LTLIBRARIES = pg_wcs.la pg_wcs2.la pg_astrolib.la 

pg_wcs_la_SOURCES = pg_wcs.c
pg_wcs_la_LDFLAGS = -module @LIB_WCS@ @LIB_M@

pg_wcs2_la_SOURCES = pg_wcs2.c
pg_wcs2_la_LDFLAGS = -module @LIB_M@

pg_astrolib_la_SOURCES = pg_astrolib.c
pg_astrolib_la_LDFLAGS = -module @LIB_NOVA@ @LIB_M@

endif
