# $Id$

bin_PROGRAMS = rts2-centrald rts2-state

noinst_HEADERS = riseset.h

rts2_centrald_SOURCES = centrald.cpp riseset.c
rts2_centrald_LDFLAGS = ../utils/librts2block.a @LIB_NOVA@ @LIB_PTHREAD@

rts2_state_SOURCES = rts2-state.c riseset.c
rts2_state_LDFLAGS = ../utils/libdevser.a @LIB_NOVA@ @LIB_PTHREAD@
