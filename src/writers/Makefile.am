# $Id$

noinst_LIBRARIES = librts2image.a librts2imagedb.a

noinst_HEADERS = rts2image.h rts2devcliimg.h rts2imagedb.h

librts2image_a_SOURCES = rts2image.cpp rts2devcliimg.cpp

librts2imagedb_a_SOURCES = rts2image.cpp rts2devcliimg.cpp rts2imagedb.cpp
librts2imagedb_a_CXXFLAGS = @LIBPG_CFLAGS@

EXTRA_DIST = rts2imagedb.ec

%.cpp: %.ec
	@ECPG@ -o $@ $^
