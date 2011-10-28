# $Id$

bin_PROGRAMS = rts2-centrald rts2-state rts2-moodd

noinst_HEADERS = centrald.h 

rts2_centrald_SOURCES = centrald.cpp
rts2_centrald_LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
rts2_centrald_CXXFLAGS = @NOVA_CFLAGS@

rts2_moodd_SOURCES = moodd.cpp
rts2_moodd_LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
rts2_moodd_CXXFLAGS = @NOVA_CFLAGS@

rts2_state_SOURCES = stateapp.cpp
rts2_state_LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
rts2_state_CXXFLAGS = @NOVA_CFLAGS@
