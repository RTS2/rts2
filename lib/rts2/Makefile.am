# Process this file with automake to produce Makefile.in

SUBDIRS = vermes  

AM_CXXFLAGS=@NOVA_CFLAGS@ -I../../include
AM_CFLAGS=@NOVA_CFLAGS@ -I../../include

EXTRA_DIST =

lib_LTLIBRARIES = librts2.la librts2users.la librts2gpib.la

librts2_la_SOURCES = hoststring.cpp app.cpp block.cpp daemon.cpp device.cpp multidev.cpp option.cpp \
	networkaddress.cpp connuser.cpp client.cpp command.cpp value.cpp valuestat.cpp \
	devclient.cpp utilsfunc.cpp iniparser.cpp configuration.cpp connnosend.cpp \
	connfork.cpp objectcheck.cpp libnova_cpp.cpp timestamp.cpp askchoice.cpp \
	message.cpp conntcp.cpp connnotify.cpp connudp.cpp connection.cpp logstream.cpp centralstate.cpp \
	rts2target.cpp displayvalue.cpp scriptdevice.cpp \
	cliapp.cpp valueminmax.cpp expander.cpp \
	riseset.cpp valuerectangle.cpp data.cpp radecparser.cpp \
	connserial.cpp connmodbus.cpp rts2format.cpp valuearray.cpp \
	connopentpl.cpp connford.cpp expression.cpp nan.c connbait.cpp \
	camd.cpp sensord.cpp filterd.cpp focusd.cpp mirror.cpp dome.cpp cupola.cpp domeford.cpp phot.cpp rotad.cpp \
	tgdrive.cpp clicupola.cpp cliwheel.cpp clifocuser.cpp slitazimuth.c connthorlabs.cpp \
	dirsupport.cpp 

librts2gpib_la_SOURCES = sensorgpib.cpp conngpib.cpp conngpibenet.cpp conngpibprologix.cpp conngpibserial.cpp

if GPIBLIB
librts2gpib_la_SOURCES += conngpiblinux.cpp
else
EXTRA_DIST += conngpiblinux.cpp
endif
	
if SUNCYG
EXTRA_DIST += OakFeatureReports.c OakHidBase.c
else
librts2_la_SOURCES += OakFeatureReports.c OakHidBase.c
endif


librts2users_la_SOURCES = userlogins.cpp

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

noinst_HEADERS = connepics.h \
	cliwheel.h clifocuser.h

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
