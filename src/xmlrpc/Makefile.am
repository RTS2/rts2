bin_PROGRAMS = rts2-xmlrpcd rts2-xmlrpcclient

noinst_HEADERS = xmlstream.h xmlrpcd.h httpreq.h r2x.h session.h stateevents.h valueevents.h events.h plot.h altplot.h \
	valueplot.h emailaction.h altaz.h augerreq.h nightdur.h nightreq.h imgpreview.h obsreq.h targetreq.h dirsupport.h \
	devicesreq.h planreq.h graphreq.h bsc.h libjavascript.h libcss.h

AM_LDFLAGS = @LIB_M@ @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ @LIBARCHIVE_CFLAGS@ -I../../lib/

if PGSQL

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp httpreq.cpp session.cpp events.cpp stateevents.cpp stateeventsdb.cpp valueevents.cpp \
	valueeventsdb.cpp emailaction.cpp plot.cpp altplot.cpp valueplot.cpp altaz.cpp augerreq.cpp nightdur.cpp \
	nightreq.cpp imgpreview.cpp obsreq.cpp targetreq.cpp dirsupport.cpp devicesreq.cpp planreq.cpp graphreq.cpp \
	libjavascript.cpp libcss.cpp
rts2_xmlrpcd_CXXFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ ${AM_CXXFLAGS}
rts2_xmlrpcd_LDFLAGS = ../scheduler/librts2scheduler.a ../db/simbad/libsimbad.a ../utilsdb/librts2blockdb.a \
	../writers/librts2imagedb.a ../utils/librts2block.a ../../lib/xmlrpc++/librts2xmlrpc.a @LIBPG_LIBS@ \
	@LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@ @LIBXML_LIBS@ @LIBARCHIVE_LIBS@ $(AM_LDFLAGS)

CLEANFILES = stateeventsdb.cpp valueeventsdb.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp httpreq.cpp session.cpp events.cpp stateevents.cpp valueevents.cpp emailaction.cpp \
	plot.cpp altaz.cpp nightdur.cpp imgpreview.cpp targetreq.cpp dirsupport.cpp devicesreq.cpp libjavascript.cpp \
	libcss.cpp
rts2_xmlrpcd_CXXFLAGS = @CFITSIO_CFLAGS@ ${AM_CXXFLAGS}
rts2_xmlrpcd_LDFLAGS = ../db/simbad/libsimbad.a ../writers/librts2image.a ../utils/librts2block.a xmlrpc++/librts2xmlrpc.a \
	@LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@ @LIBXML_LIBS@ @LIBARCHIVE_LIBS@ $(AM_LDFLAGS)

endif

EXTRA_DIST = stateeventsdb.ec valueeventsdb.ec graphreq.cpp

rts2_xmlrpcclient_SOURCES = xmlrpcclient.cpp
rts2_xmlrpcclient_CXXFLAGS = @NOVA_CFLAGS@ ${AM_CXXFLAGS}
rts2_xmlrpcclient_LDFLAGS = ../utils/librts2block.a ../../lib/xmlrpc++/librts2xmlrpc.a @LIB_NOVA@ $(AM_LDFLAGS)
