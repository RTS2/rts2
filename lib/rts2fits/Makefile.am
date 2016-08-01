lib_LTLIBRARIES = librts2image.la 

LDADD = @LIB_M@ @LIB_NOVA@

EXTRA_DIST = imagedb.ec dbfilters.ec

CLEANFILES = imagedb.cpp dbfilters.cpp

librts2image_la_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp imageprocess.cpp
librts2image_la_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ -I../../include
librts2image_la_LIBADD = ../rts2/librts2.la @CFITSIO_LIBS@ @MAGIC_LIBS@

if PGSQL

lib_LTLIBRARIES += librts2imagedb.la

nodist_librts2imagedb_la_SOURCES = imagedb.cpp
librts2imagedb_la_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ -I../../include
librts2imagedb_la_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp dbfilters.cpp
librts2imagedb_la_LIBADD = ../rts2db/librts2db.la

.ec.cpp:
	@ECPG@ -o $@ $^

endif
