# Process this file with automake to produce Makefile.in

noinst_PROGRAMS = test

bin_PROGRAMS=rts2-horizon

AM_CXXFLAGS=@NOVA_CFLAGS@

test_SOURCES = test.cpp mkpath.c
test_LDFLAGS = @LIB_NOVA@ librts2block.a

noinst_LIBRARIES = librts2block.a

librts2block_a_SOURCES = rts2app.cpp rts2block.cpp rts2daemon.cpp rts2device.cpp rts2option.cpp \
	rts2address.cpp rts2user.cpp rts2client.cpp rts2command.cpp rts2value.cpp \
	rts2devclient.cpp rts2dataconn.cpp mkpath.c rts2config.cpp rts2connnosend.cpp \
	rts2connfork.cpp objectcheck.cpp libnova_cpp.cpp timestamp.cpp rts2askchoice.cpp \
	rts2message.cpp rts2conn.cpp rts2logstream.cpp rts2centralstate.cpp rts2scriptdevice.cpp

noinst_HEADERS = mkpath.h rts2app.h rts2block.h rts2daemon.h rts2device.h rts2option.h \
	rts2address.h rts2user.h rts2client.h rts2command.h rts2value.h \
	rts2devclient.h rts2dataconn.h rts2event.h rts2config.h rts2object.h objectcheck.h rts2connnosend.h \
	rts2connfork.h rts2serverstate.h libnova_cpp.h timestamp.h rts2askchoice.h infoval.h \
	rts2message.h rts2conn.h rts2logstream.h rts2centralstate.h rts2target.h rts2valuelist.h \
	rts2displayvalue.h rts2scriptdevice.h

rts2_horizon_SOURCES = horizonapp.cpp
rts2_horizon_LDFLAGS = @LIB_NOVA@ librts2block.a
