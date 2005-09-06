# $Id$

noinst_LIBRARIES = librts2image.a librts2imagedb.a

noinst_HEADERS = rts2image.h rts2devcliimg.h rts2imagedb.h rts2devclifoc.h rts2appdbimage.h

noinst_PROGRAMS = rts2-imgtest

librts2image_a_SOURCES = rts2image.cpp imageastrometry.cpp ham.cpp rts2devcliimg.cpp rts2devclifoc.cpp

librts2imagedb_a_SOURCES = rts2image.cpp imageastrometry.cpp ham.cpp rts2devcliimg.cpp rts2imagedb.cpp rts2devclifoc.cpp
librts2imagedb_a_CXXFLAGS = @LIBPG_CFLAGS@

rts2_imgtest_SOURCES = imgtest.cpp
rts2_imgtest_LDFLAGS = @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ librts2image.a ../utils/librts2block.a

EXTRA_DIST = rts2imagedb.ec

%.cpp: %.ec
	@ECPG@ -o $@ $^
