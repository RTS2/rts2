EXTRA_DIST = autogen.sh acinclude.m4

noinst_HEADERS = bb.h httpreq.h observatory.h

if PGSQL

bin_PROGRAMS = rts2-bb

rts2_bb_SOURCES = bb.cpp httpreq.cpp observatory.cpp
rts2_bb_CXXFLAGS = @CFITSIO_CFLAGS@ @LIBARCHIVE_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @NOVA_CFLAGS@ -I../..//include -I../../lib
rts2_bb_LDADD = -L../../lib/rts2db -lrts2db -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 -L../../lib/xmlrpc++ -lrts2xmlrpc \
	@LIBXML_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_JPEG@ @LIB_CRYPT@ @LIBARCHIVE_LIBS@ @LIB_CFITSIO@

else

EXTRA_DIST += bb.cpp httpreq.cpp observatory.cpp

endif
