# $Id$

bin_PROGRAMS = rts2-grbd rts2-grbforward

noinst_HEADERS = grbd.h grbconst.h rts2conngrb.h rts2grbfw.h
extra_DIST = rts2conngrb.ec

rts2_grbd_SOURCES = grbd.cpp rts2conngrb.cpp rts2grbfw.cpp
rts2_grbd_CXXFLAGS = @LIBPG_CFLAGS@
rts2_grbd_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_grbforward_SOURCES = forward.cpp rts2grbfw.cpp
rts2_grbforward_LDFLAGS = @LIB_M@ ../utils/librts2block.a

%.cpp: %.ec
	@ECPG@ -o $@ $^
