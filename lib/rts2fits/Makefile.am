lib_LTLIBRARIES = librts2image.la 

noinst_HEADERS = memimage.h

LDADD = @LIB_M@ @LIB_NOVA@

EXTRA_DIST = imagedb.ec dbfilters.ec

CLEANFILES = imagedb.cpp dbfilters.cpp

librts2image_la_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp imageprocess.cpp memimage.cpp
librts2image_la_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

if PGSQL

lib_LTLIBRARIES += librts2imagedb.la

nodist_librts2imagedb_la_SOURCES = imagedb.cpp
librts2imagedb_la_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
librts2imagedb_la_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp dbfilters.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
