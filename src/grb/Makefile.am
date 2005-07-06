# $Id$

bin_PROGRAMS = rts2-grbd

noinst_HEADERS = grbd.h grbconst.h rts2conngrb.h
extra_DIST = rts2conngrb.ec

rts2_grbd_SOURCES = grbd.cpp rts2conngrb.cpp
rts2_grbd_CXXFLAGS = @LIBPG_CFLAGS@
rts2_grbd_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a

%.cpp: %.ec
	@ECPG@ -o $@ $^
