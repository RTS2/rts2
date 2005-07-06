# $Id$

bin_PROGRAMS = rts2-grbc

noinst_HEADERS = grbc.h grbconst.h rts2conngrb.h
extra_DIST = rts2conngrb.ec

rts2_grbc_SOURCES = grbc.cpp rts2conngrb.cpp
rts2_grbc_CXXFLAGS = @LIBPG_CFLAGS@
rts2_grbc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a

%.cpp: %.ec
	@ECPG@ -o $@ $^
