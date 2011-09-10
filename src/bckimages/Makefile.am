bin_PROGRAMS = rts2-flatprocess

EXTRA_DIST = bckimages.ec deleteimage.ec

CLEANFILES = bckimages.cpp deleteimage.cpp

AM_CFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@
AM_CXXFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@

rts2_flatprocess_SOURCES = flatprocess.cpp
rts2_flatprocess_LDADD = -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2 -lrts2 @LIB_CFITSIO@ @LIB_NOVA@ @LIB_M@

if PGSQL

bin_PROGRAMS += rts2-bckimages rts2-deleteimage

nodist_rts2_bckimages_SOURCES = bckimages.cpp
rts2_bckimages_CXXFLAGS = @LIBPG_CFLAGS@ @JPEG_CFLAGS@
rts2_bckimages_LDADD = -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2db -lrts2db -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 @LDFLAGS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@

nodist_rts2_deleteimage_SOURCES = deleteimage.cpp
rts2_deleteimage_CXXFLAGS = @LIBPG_CFLAGS@ @JPEG_CFLAGS@ @CFITSIO_CFLAGS@
rts2_deleteimage_LDADD = -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2db -lrts2db -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 @LDFLAGS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
