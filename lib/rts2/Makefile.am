# Process this file with automake to produce Makefile.in

SUBDIRS = model

noinst_PROGRAMS = test

bin_PROGRAMS=rts2-horizon

AM_CXXFLAGS=@NOVA_CFLAGS@

test_SOURCES = test.cpp
test_LDADD = -lrts2 @LIB_NOVA@

lib_LTLIBRARIES = librts2.la

librts2_la_SOURCES = hoststring.cpp app.cpp block.cpp daemon.cpp device.cpp option.cpp \
	rts2address.cpp connuser.cpp client.cpp command.cpp value.cpp valuestat.cpp \
	devclient.cpp utilsfunc.cpp iniparser.cpp configuration.cpp connnosend.cpp \
	connfork.cpp objectcheck.cpp libnova_cpp.cpp timestamp.cpp rts2askchoice.cpp \
	message.cpp conntcp.cpp connnotify.cpp connudp.cpp connection.cpp logstream.cpp rts2centralstate.cpp \
	rts2target.cpp displayvalue.cpp scriptdevice.cpp \
	rts2cliapp.cpp valueminmax.cpp expander.cpp \
	riseset.cpp valuerectangle.cpp rts2data.cpp radecparser.cpp \
	connserial.cpp connmodbus.cpp rts2format.cpp valuearray.cpp \
	connopentpl.cpp connford.cpp expression.cpp nan.c connbait.cpp \
	teld.cpp camd.cpp sensord.cpp filterd.cpp focusd.cpp mirror.cpp dome.cpp domeford.cpp phot.cpp rotad.cpp \
	gem.cpp fork.cpp tgdrive.cpp \
	clicupola.cpp cliwheel.cpp clifocuser.cpp

librts2_la_LIBADD = model/librts2model.la

EXTRA_DIST =

# CYGWIN workaround
if NOT_GETADDRINFO
librts2_la_SOURCES += getaddrinfo.c
else
EXTRA_DIST += getaddrinfo.c
endif

if NOT_GETOPT_LONG
librts2_la_SOURCES += getopt_own.c
else
EXTRA_DIST += getopt_own.c
endif

noinst_HEADERS = client.h rts2askchoice.h infoval.h \
	conntcp.h connudp.h rts2centralstate.h rts2target.h \
	displayvalue.h rts2cliapp.h getaddrinfo.h expander.h \
	connserial.h connmodbus.h domeford.h \
	connopentpl.h connepics.h connford.h counted_ptr.h expression.h \
	gem.h fork.h tgdrive.h \
	connbait.h clicupola.h cliwheel.h clifocuser.h

if EPICS
lib_LTLIBRARIES += librts2epics.la
librts2epics_la_SOURCES = connepics.cpp
librts2epics_la_CXXFLAGS = @INC_EPICS@ @NOVA_CFLAGS@
else
EXTRA_DIST += connepics.cpp
endif

if SUNOS
EXTRA_DIST += nimotion.c
else
librts2_la_SOURCES += nimotion.c
endif

rts2_horizon_SOURCES = horizonapp.cpp
rts2_horizon_LDADD = -lrts2 @LIB_NOVA@
