# $Id$

noinst_LIBRARIES = libdb.a

libdb_a_SOURCES = db.c
AM_CFLAGS = @LIBPG_CFLAGS@

%.c: %.ec
	@ECPG@ -o $@ $^
