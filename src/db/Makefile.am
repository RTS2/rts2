# $Id$

bin_PROGRAMS = rts2-photlist

rts2_photlist_SOURCES = photlist.c
rts2_photlist_LDFLAGS = @LIBPG_CFLAGS@ @LIB_ECPG@

noinst_LIBRARIES = libdb.a
libdb_a_SOURCES = db.c
noinst_HEADERS = db.h

EXTRA_DIST = db.ec

AM_CFLAGS = @LIBPG_CFLAGS@

%.c: %.ec
	@ECPG@ -o $@ $^

noinst_PROGRAMS = test

bin_SCRIPTS = rts2-script

test_SOURCES = test.c
test_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ libdb.a
