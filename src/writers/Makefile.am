noinst_LIBRARIES = librts2image.a 

noinst_HEADERS = fitsfile.h channel.h image.h devcliimg.h cameraimage.h imagedb.h devclifoc.h rts2appdbimage.h appimage.h memimage.h

LDADD = @LIB_M@ @LIB_NOVA@

EXTRA_DIST = imagedb.ec

CLEANFILES = imagedb.cpp

librts2image_a_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp ham.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp imageprocess.cpp memimage.cpp
librts2image_a_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

bin_PROGRAMS = rts2-image

rts2_image_SOURCES = appimagemanip.cpp imageprocess.cpp

if PGSQL

noinst_LIBRARIES += librts2imagedb.a


nodist_librts2imagedb_a_SOURCES = imagedb.cpp
librts2imagedb_a_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
librts2imagedb_a_SOURCES = fitsfile.cpp channel.cpp image.cpp imageastrometry.cpp ham.cpp devcliimg.cpp cameraimage.cpp devclifoc.cpp

rts2_image_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
rts2_image_LDADD = ../utilsdb/librts2blockdb.a librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

rts2_image_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
rts2_image_LDFLAGS = librts2image.a ../utils/librts2block.a ../utils/utilsfunc.o @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

endif
