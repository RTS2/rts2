# $Id$

bin_PROGRAMS = rts2-mirror-fram rts2-mirror-dummy

noinst_HEADERS = mirror.h

EXTRA_DIST = mirror.cpp

LDADD = mirror.o ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

rts2_mirror_fram_SOURCES = fram.cpp

rts2_mirror_dummy_SOURCES = dummy.cpp
