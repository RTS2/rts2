# $Id$


bin_PROGRAMS = rts2-focusc

if XFOCUSC
	bin_PROGRAMS += rts2-xfocusc
	rts2_xfocusc_SOURCES = xfocusc.cpp
	rts2_xfocusc_LDFLAGS = @X_LIBS@ @X_CFLAGS@ -lX11 @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../utils/libdevcli.a ../writers/librts2fits.a ../utils/config.o 
endif

rts2_focusc_SOURCES = focusc.c
rts2_focusc_LDFLAGS = @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../utils/libdevcli.a ../writers/librts2fits.a ../utils/config.o
