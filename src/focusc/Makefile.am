# $Id$

bin_PROGRAMS = rts2-focusc

rts2_focusc_SOURCES = focusc.cpp
rts2_focusc_LDFLAGS = @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../writers/librts2image.a ../utils/librts2block.a

if XFOCUSC
bin_PROGRAMS += rts2-xfocusc
rts2_xfocusc_SOURCES = xfocusc.cpp
rts2_xfocusc_LDFLAGS = @X_LIBS@ @X_CFLAGS@ -lX11 @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../writers/librts2image.a ../utils/librts2block.a 
endif
