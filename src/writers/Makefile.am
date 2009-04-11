noinst_LIBRARIES = librts2image.a 

noinst_HEADERS = rts2fitsfile.h rts2image.h rts2devcliimg.h cameraimage.h rts2imagedb.h rts2devclifoc.h rts2appdbimage.h rts2appimage.h

LDADD = @LIB_M@ @LIB_NOVA@

EXTRA_DIST = rts2imagedb.ec

CLEANFILES = rts2imagedb.cpp

librts2image_a_SOURCES = rts2fitsfile.cpp rts2image.cpp imageastrometry.cpp ham.cpp rts2devcliimg.cpp cameraimage.cpp rts2devclifoc.cpp imageprocess.cpp
librts2image_a_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@

bin_PROGRAMS = rts2-image

rts2_image_SOURCES = appimagemanip.cpp imageprocess.cpp

if PGSQL

noinst_LIBRARIES += librts2imagedb.a


nodist_librts2imagedb_a_SOURCES = rts2imagedb.cpp
librts2imagedb_a_SOURCES = rts2fitsfile.cpp rts2image.cpp imageastrometry.cpp ham.cpp rts2devcliimg.cpp cameraimage.cpp rts2devclifoc.cpp
librts2imagedb_a_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@

rts2_image_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_image_LDFLAGS = ../utilsdb/librts2blockdb.a librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

rts2_image_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_image_LDFLAGS = librts2image.a ../utils/librts2block.a ../utils/utilsfunc.o @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

endif
