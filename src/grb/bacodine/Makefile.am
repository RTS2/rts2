# $Id$

bin_PROGRAMS = rts2-bacmail

noinst_HEADERS = bacodine.h

rts2_bacmail_SOURCES = bacmail.c
rts2_bacmail_LDFLAGS = @LIB_ECPG@ ../../db/libdb.a ../../utils/config.o
rts2_bacmail_CFLASG = @LIBPG_CFLAGS@

noinst_LIBRARIES = libbacodine.a

libbacodine_a_SOURCES = bacodine.c bacoclient.c bacocore.c
