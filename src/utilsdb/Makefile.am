# Process this file with automake to produce Makefile.in

noinst_LIBRARIES = librts2blockdb.a

librts2blockdb_a_SOURCES = rts2devicedb.cpp target.cpp sub_targets.cpp rts2appdb.cpp
librts2blockdb_a_CXXFLAGS = @LIBPG_CFLAGS@

noinst_HEADERS = rts2devicedb.h target.h rts2appdb.h

EXTRA_DIST = rts2devicedb.ec target.ec sub_targets.ec rts2appdb.ec

%.cpp: %.ec
	@ECPG@ -o $@ $^
