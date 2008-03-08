# $Id$

bin_PROGRAMS = rts2-centrald rts2-state

noinst_HEADERS = centrald.h 

rts2_centrald_SOURCES = centrald.cpp
rts2_centrald_LDFLAGS = ../utils/librts2block.a @LIB_NOVA@ @LIB_PTHREAD@
rts2_centrald_CXXFLAGS = @NOVA_CFLAGS@

rts2_state_SOURCES = rts2-state.cpp
rts2_state_LDFLAGS = ../utils/librts2block.a @LIB_NOVA@ @LIB_PTHREAD@
rts2_state_CXXFLAGS = @NOVA_CFLAGS@
