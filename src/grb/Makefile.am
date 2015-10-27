bin_PROGRAMS = rts2-grbforward

noinst_HEADERS = grbd.h grbconst.h conngrb.h rts2grbfw.h connshooter.h augershooter.h

EXTRA_DIST = conngrb.ec connshooter.ec

CLEANFILES = conngrb.cpp connshooter.cpp

rts2_grbforward_SOURCES = forward.cpp rts2grbfw.cpp
rts2_grbforward_LDADD = -L../../lib/rts2 -lrts2 @LIB_M@ @LIB_NOVA@
rts2_grbforward_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

if PGSQL

bin_PROGRAMS += rts2-grbd rts2-augershooter

PG_CXXFLAGS = @CFITSIO_CFLAGS@ @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ -I../../include
PG_LDADD = -L../../lib/rts2script -lrts2script -L../../lib/rts2db -lrts2db -L../../lib/pluto -lpluto -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_CRYPT@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

nodist_rts2_grbd_SOURCES = conngrb.cpp
rts2_grbd_SOURCES = grbd.cpp rts2grbfw.cpp
rts2_grbd_CXXFLAGS = ${PG_CXXFLAGS} -I../../include
rts2_grbd_LDADD = ${PG_LDADD}

nodist_rts2_augershooter_SOURCES = connshooter.cpp
rts2_augershooter_SOURCES = augershooter.cpp
rts2_augershooter_CXXFLAGS = ${PG_CXXFLAGS} -I../../include
rts2_augershooter_LDADD = ${PG_LDADD}

.ec.cpp:
	@ECPG@ -o $@ $^

endif
