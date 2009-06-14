# $Id$

bin_PROGRAMS = rts2-focusc rts2-foctest

noinst_HEADERS = genfoc.h

EXTRA_DIST = genfoc.cpp

AM_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

rts2_focusc_SOURCES = focusc.cpp
rts2_focusc_LDFLAGS = ../writers/librts2image.a ../utils/librts2block.a @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ @LIB_JPEG@
rts2_focusc_LDADD = genfoc.o

rts2_foctest_SOURCES = foctest.cpp
rts2_foctest_LDFLAGS = ../writers/librts2image.a ../utils/librts2block.a @LIB_CFITSIO@ @LIB_M@ @LIB_NOVA@ @LIB_JPEG@

if XFOCUSC
bin_PROGRAMS += rts2-xfocusc
rts2_xfocusc_SOURCES = xfocusc.cpp
rts2_xfocusc_LDFLAGS = ../writers/librts2image.a ../utils/librts2block.a @X_LIBS@ @X_CFLAGS@ -lX11 @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ @LIB_JPEG@
rts2_xfocusc_LDADD = genfoc.o
endif
