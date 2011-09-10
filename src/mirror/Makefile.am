# $Id$

bin_PROGRAMS = rts2-mirror-dummy

EXTRA_DIST = mirror.cpp

LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

if SUNOS
EXTRA_DIST += fram.cpp
else

bin_PROGRAMS += rts2-mirror-fram

rts2_mirror_fram_SOURCES = fram.cpp
endif

rts2_mirror_dummy_SOURCES = dummy.cpp
