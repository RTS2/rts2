# $Id$

inst_LTLIBRARIES = pg_wcs.la pg_astrolib.la

instdir = @PG_CONFIG_SERVER_LIBS@

pg_wcs_la_SOURCES = pg_wcs.c
pg_wcs_la_CFLAGS = @LIBPG_CFLAGS@ 
pg_wcs_la_LDFLAGS = -module @LIB_WCS@

pg_astrolib_la_SOURCES = pg_astrolib.c
pg_astrolib_la_CFLAGS = @LIBPG_CFLAGS@
pg_astrolib_la_LDFLAGS = -module @LIB_NOVA@
