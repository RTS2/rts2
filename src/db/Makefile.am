# $Id$

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
