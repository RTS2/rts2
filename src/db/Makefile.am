# $Id$

noinst_LIBRARIES = libdb.a

libdb_a_SOURCES = db.c
AM_CFLAGS = @LIBPG_CFLAGS@

%.c: %.ec
	@ECPG@ -o $@ $^

noinst_PROGRAMS = test

test_SOURCES = test.c
test_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ libdb.a
