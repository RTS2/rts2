# $Id$

bin_PROGRAMS = rts2-flatprocess

EXTRA_DIST = bckimages.ec

AM_CFLAGS = @LIBPG_CFLAGS@

rts2_flatprocess_SOURCES = flatprocess.cpp
rts2_flatprocess_LDFLAGS = ../utils/librts2block.a @LIB_CFITSIO@ @LIB_NOVA@ @LIB_M@

if PGSQL

bin_PROGRAMS += rts2-bckimages rts2-deleteimage

rts2_bckimages_SOURCES = bckimages.c
rts2_bckimages_LDFLAGS = ../utils/librts2block.a @LDFLAGS@ @LIBPG_LIBS@ @LIB_ECPG@ 

rts2_deleteimage_SOURCES = deleteimage.cpp
rts2_deleteimage_CXXFLAGS = @LIBPG_CFLAGS@
rts2_deleteimage_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LDFLAGS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@

%.c: %.ec
	@ECPG@ -o $@ $^

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

EXTRA_DIST += deleteimage.cpp

endif
