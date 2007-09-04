# $Id$

bin_PROGRAMS = rts2-bckimages rts2-flatprocess rts2-deleteimage

rts2_bckimages_SOURCES = bckimages.c

EXTRA_DIST = bckimages.ec

rts2_flatprocess_SOURCES = flatprocess.cpp

# AM_CFLAGS = @LIBPG_CFLAGS@ -I@top_srcdir@/include
AM_CFLAGS = @LIBPG_CFLAGS@

rts2_bckimages_LDFLAGS = ../utils/librts2block.a @LDFLAGS@ @LIBPG_LIBS@ @LIB_ECPG@ 
rts2_flatprocess_LDFLAGS = ../utils/librts2block.a @LIB_CFITSIO@ @LIB_NOVA@ @LIB_M@

rts2_deleteimage_SOURCES = deleteimage.cpp
rts2_deleteimage_CXXFLAGS = @LIBPG_CFLAGS@
rts2_deleteimage_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LDFLAGS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@

%.c: %.ec
	@ECPG@ -o $@ $^

%.cpp: %.ec
	@ECPG@ -o $@ $^
