# $Id$

noinst_LIBRARIES = libdb.a

libdb_a_SOURCES = db.c
libdb_a_CFLAGS = @LIBPG_CFLAGS@

%.c: %.ec
	@ECPG@ -o $@ $^
