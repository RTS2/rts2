# Process this file with automake to produce Makefile.in

noinst_PROGRAMS = test

bin_PROGRAMS=rts2-horizon

AM_CXXFLAGS=@NOVA_CFLAGS@

test_SOURCES = test.cpp utilsfunc.cpp
test_LDADD = librts2block.a @LIB_NOVA@

noinst_LIBRARIES = librts2block.a

librts2block_a_SOURCES = hoststring.cpp app.cpp block.cpp daemon.cpp device.cpp rts2option.cpp \
	rts2address.cpp rts2user.cpp rts2client.cpp rts2command.cpp value.cpp valuestat.cpp \
	rts2devclient.cpp utilsfunc.cpp rts2configraw.cpp rts2config.cpp rts2connnosend.cpp \
	connfork.cpp objectcheck.cpp libnova_cpp.cpp timestamp.cpp rts2askchoice.cpp \
	rts2message.cpp conntcp.cpp connudp.cpp rts2conn.cpp rts2logstream.cpp rts2centralstate.cpp \
	rts2target.cpp rts2displayvalue.cpp scriptdevice.cpp \
	rts2cliapp.cpp valueminmax.cpp expander.cpp \
	riseset.cpp valuerectangle.cpp rts2data.cpp radecparser.cpp \
	connserial.cpp connmodbus.cpp rts2format.cpp valuearray.cpp \
	connopentpl.cpp connford.cpp expression.cpp nan.c

EXTRA_DIST =

# CYGWIN workaround
if NOT_GETADDRINFO
librts2block_a_SOURCES += getaddrinfo.c
else
EXTRA_DIST += getaddrinfo.c
endif

if NOT_GETOPT_LONG
librts2block_a_SOURCES += getopt_own.c
else
EXTRA_DIST += getopt_own.c
endif

noinst_HEADERS = hoststring.h utilsfunc.h app.h block.h daemon.h device.h rts2option.h \
	rts2address.h rts2user.h rts2client.h rts2command.h value.h valuestat.h \
	rts2devclient.h rts2event.h rts2configraw.h rts2config.h rts2object.h rts2connnosend.h \
	connfork.h objectcheck.h rts2serverstate.h libnova_cpp.h timestamp.h rts2askchoice.h infoval.h \
	rts2message.h conntcp.h connudp.h rts2conn.h rts2logstream.h rts2centralstate.h rts2target.h valuelist.h \
	rts2displayvalue.h scriptdevice.h rts2cliapp.h getaddrinfo.h valueminmax.h expander.h \
	riseset.h valuerectangle.h rts2data.h radecparser.h connserial.h connmodbus.h rts2format.h \
	valuearray.h getopt_own.h connopentpl.h connepics.h error.h connford.h counted_ptr.h expression.h nan.h

if EPICS
noinst_LIBRARIES += librts2epics.a
librts2epics_a_SOURCES = connepics.cpp
librts2epics_a_CXXFLAGS = @INC_EPICS@ @NOVA_CFLAGS@
else
EXTRA_DIST += connepics.cpp
endif

rts2_horizon_SOURCES = horizonapp.cpp
rts2_horizon_LDADD = librts2block.a @LIB_NOVA@
