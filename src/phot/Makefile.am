# $Id$

bin_PROGRAMS = rts2-phot-optec rts2-phot-dummy

noinst_HEADERS = phot.h kernel/phot.h

EXTRA_DIST = phot.cpp

LDADD = phot.o ../utils/librts2block.a @LIB_NOVA@

AM_CXXFLAGS=@NOVA_CFLAGS@

rts2_phot_optec_SOURCES = phot_optec.cpp

rts2_phot_dummy_SOURCES = dummy.cpp
