# $Id$

bin_PROGRAMS = rts2-phot-ssp5 rts2-phot-dummy

noinst_HEADERS = phot.h kernel/phot.h

noinst_LIBRARIES = librts2phot.a

LDADD = librts2phot.a ../utils/librts2block.a @LIB_NOVA@

AM_CXXFLAGS=@NOVA_CFLAGS@

librts2phot_a_SOURCES = phot.cpp

if SUNOS
EXTRA_DIST = phot_optec.cpp
else
bin_PROGRAMS += rts2-phot-optec
rts2_phot_optec_SOURCES = phot_optec.cpp
endif

rts2_phot_ssp5_SOURCES = ssp5.cpp

rts2_phot_dummy_SOURCES = dummy.cpp
