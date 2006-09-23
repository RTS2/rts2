# $Id$

noinst_LIBRARIES = librts2image.a librts2imagedb.a

noinst_HEADERS = rts2image.h rts2devcliimg.h rts2imagedb.h rts2devclifoc.h rts2appdbimage.h rts2appimage.h

bin_PROGRAMS = rts2-image

librts2image_a_SOURCES = rts2image.cpp imageastrometry.cpp ham.cpp rts2devcliimg.cpp rts2devclifoc.cpp

librts2imagedb_a_SOURCES = rts2image.cpp imageastrometry.cpp ham.cpp rts2devcliimg.cpp rts2imagedb.cpp rts2devclifoc.cpp
librts2imagedb_a_CXXFLAGS = @LIBPG_CFLAGS@

rts2_image_SOURCES = appimagemanip.cpp
rts2_image_CXXFLAGS = @LIBPG_CFLAGS@
rts2_image_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a librts2imagedb.a ../utils/librts2block.a

EXTRA_DIST = rts2imagedb.ec

%.cpp: %.ec
	@ECPG@ -o $@ $^
