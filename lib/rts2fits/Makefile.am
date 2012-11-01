lib_LTLIBRARIES = librts2image.la 

LDADD = @LIB_M@ @LIB_NOVA@

EXTRA_DIST = imagedb.ec

CLEANFILES = imagedb.cpp

librts2image_la_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp imageprocess.cpp
librts2image_la_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

if PGSQL

lib_LTLIBRARIES += librts2imagedb.la

nodist_librts2imagedb_la_SOURCES = imagedb.cpp
librts2imagedb_la_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
librts2imagedb_la_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
