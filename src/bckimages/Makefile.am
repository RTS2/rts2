bin_PROGRAMS = rts2-flatprocess

EXTRA_DIST = bckimages.ec deleteimage.ec

CLEANFILES = bckimages.cpp deleteimage.cpp

AM_CFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ -I../../include
AM_CXXFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ -I../../include

rts2_flatprocess_SOURCES = flatprocess.cpp
rts2_flatprocess_LDADD = -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2 -lrts2 @CFITSIO_LIBS@ @LIB_NOVA@ @LIB_M@

if PGSQL

bin_PROGRAMS += rts2-bckimages rts2-deleteimage

PG_LDADD = -L../../lib/rts2script -lrts2script -L../../lib/rts2db -lrts2db -L../../lib/pluto -lpluto -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 -L../../lib/xmlrpc++ -lrts2xmlrpc @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_CRYPT@ @LIB_NOVA@ @CFITSIO_LIBS@ @LIB_M@ @MAGIC_LIBS@

nodist_rts2_bckimages_SOURCES = bckimages.cpp
rts2_bckimages_CXXFLAGS = @LIBPG_CFLAGS@ @MAGIC_CFLAGS@ @LIBXML_LIBS@ -I../../include
rts2_bckimages_LDADD = ${PG_LDADD}

nodist_rts2_deleteimage_SOURCES = deleteimage.cpp
rts2_deleteimage_CXXFLAGS = @LIBPG_CFLAGS@ @MAGIC_CFLAGS@ @CFITSIO_CFLAGS@ @LIBXML_LIBS@ -I../../include
rts2_deleteimage_LDADD = ${PG_LDADD}

.ec.cpp:
	@ECPG@ -o $@ $^

endif
