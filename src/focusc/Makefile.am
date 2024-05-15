bin_PROGRAMS = rts2-focusc rts2-foctest

noinst_HEADERS = focusclient.h xfocusc.h xfitsimage.h
noinst_LIBRARIES = libfocusclient.a

libfocusclient_a_SOURCES = focusclient.cpp

AM_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ @NCURSESW_CFLAGS@ -I../../include

rts2_focusc_SOURCES = focusc.cpp
rts2_focusc_LDADD = libfocusclient.a -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2 @LIB_M@ @CFITSIO_LIBS@ @LIB_NOVA@ @MAGIC_LIBS@ @NCURSESW_LIBS@

rts2_foctest_SOURCES = foctest.cpp
rts2_foctest_LDADD = -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2 @CFITSIO_LIBS@ @LIB_M@ @LIB_NOVA@ @MAGIC_LIBS@

if XFOCUSC
bin_PROGRAMS += rts2-xfocusc
rts2_xfocusc_SOURCES = xfocusc.cpp xfitsimage.cpp
rts2_xfocusc_LDADD = libfocusclient.a -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2 @X_LIBS@ @X_CFLAGS@ -lX11 @LIB_M@ @CFITSIO_LIBS@ @LIB_NOVA@ @MAGIC_LIBS@ @NCURSESW_LIBS@
endif
