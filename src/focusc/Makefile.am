# $Id$

bin_PROGRAMS = rts2-focusc rts2-xfocusc

rts2_focusc_SOURCES = focusc.c
rts2_focusc_LDFLAGS = @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../utils/libdevcli.a ../writers/librts2fits.a ../utils/config.o

rts2_xfocusc_SOURCES = xfocusc.c
rts2_xfocusc_LDFLAGS = -L/usr/X11R6/lib -I/usr/X11R6/include -lX11 @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../utils/libdevcli.a ../writers/librts2fits.a ../utils/config.o 
