# $Id$

bin_PROGRAMS = rts2-planc

noinst_PROGRAMS = test

noinst_HEADERS = selector.h median.h

rts2_planc_SOURCES = planc.cpp selector.cpp target.cpp median.c centering.cpp
test_SOURCES = test.cpp target.cpp selector.cpp median.c

EXTRA_DIST = selector.ec

rts2_planc_CXXFLAGS = @LIBPG_CFLAGS@

rts2_planc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_PTHREAD@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2fits.a ../utils/libdevcli.a ../db/libdb.a

test_CXXFLAGS = @LIBPG_CFLAGS@

test_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_PTHREAD@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2fits.a ../utils/libdevcli.a ../db/libdb.a

%.cpp: %.ec
	@ECPG@ -o $@ $^
