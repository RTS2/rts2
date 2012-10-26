EXTRA_DIST = bbdb.ec

bbsdir = ${datadir}/@PACKAGE@/bb

dist_bbs_SCRIPTS = create_target.py

noinst_HEADERS = bb.h bbdb.h bbapi.h observatory.h

if PGSQL

bin_PROGRAMS = rts2-bb

rts2_bb_SOURCES = bb.cpp bbdb.cpp bbapi.cpp observatory.cpp
rts2_bb_CXXFLAGS = @CFITSIO_CFLAGS@ @LIBARCHIVE_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @JSONGLIB_CFLAGS@ -I../../include -I../../lib
rts2_bb_LDADD = -L../../lib/rts2json -lrts2json -L../../lib/rts2db -lrts2db -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 -L../../lib/xmlrpc++ -lrts2xmlrpc \
	@LIBXML_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_JPEG@ @LIB_CRYPT@ @LIBARCHIVE_LIBS@ @LIB_CFITSIO@ @JSONGLIB_LIBS@

CLEANFILES = bbdb.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

EXTRA_DIST += bb.cpp httpreq.cpp bbapi.cpp observatory.cpp

endif
