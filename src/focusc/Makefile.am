# $Id$

bin_PROGRAMS = rts2-focusc rts2-foctest

noinst_HEADERS = genfoc.h

EXTRA_DIST = genfoc.cpp

rts2_focusc_SOURCES = focusc.cpp
rts2_focusc_LDFLAGS = @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../writers/librts2image.a ../utils/librts2block.a
rts2_focusc_LDADD = genfoc.o
rts2_focusc_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ 

rts2_foctest_SOURCES = foctest.cpp
rts2_foctest_LDFLAGS = @LIB_CFITSIO@ @LIB_M@ @LIB_NOVA@ ../writers/librts2image.a ../utils/librts2block.a
rts2_foctest_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@

if XFOCUSC
bin_PROGRAMS += rts2-xfocusc
rts2_xfocusc_SOURCES = xfocusc.cpp
rts2_xfocusc_LDFLAGS = @X_LIBS@ @X_CFLAGS@ -lX11 @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../writers/librts2image.a ../utils/librts2block.a 
rts2_xfocusc_LDADD = genfoc.o
rts2_xfocusc_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
endif
