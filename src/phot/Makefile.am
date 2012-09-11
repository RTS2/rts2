# $Id$

bin_PROGRAMS = rts2-phot-ssp5 rts2-phot-dummy

noinst_HEADERS = kernel/phot.h

LDADD = -lrts2 -L../../lib/rts2 @LIB_NOVA@

AM_CXXFLAGS=@NOVA_CFLAGS@ -I../../include

if SUNOS
EXTRA_DIST = phot_optec.cpp
else
bin_PROGRAMS += rts2-phot-optec
rts2_phot_optec_SOURCES = phot_optec.cpp
endif

rts2_phot_ssp5_SOURCES = ssp5.cpp

rts2_phot_dummy_SOURCES = dummy.cpp
