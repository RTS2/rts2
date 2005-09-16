# Process this file with automake to produce Makefile.in

bin_PROGRAMS = rts2-airmasscale
noinst_LIBRARIES = librts2blockdb.a

rts2_airmasscale_SOURCES = airmasscale.cpp
rts2_airmasscale_CXXFLAGS = @LIBPG_CFLAGS@
rts2_airmasscale_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ librts2blockdb.a ../utils/librts2block.a

librts2blockdb_a_SOURCES = rts2devicedb.cpp target.cpp sub_targets.cpp rts2appdb.cpp
librts2blockdb_a_CXXFLAGS = @LIBPG_CFLAGS@

noinst_HEADERS = rts2devicedb.h target.h rts2appdb.h

EXTRA_DIST = rts2devicedb.ec target.ec sub_targets.ec rts2appdb.ec airmasscale.ec

%.cpp: %.ec
	@ECPG@ -o $@ $^
