bin_PROGRAMS = rts2-xmlrpcd rts2-xmlrpcclient rts2-jsonclient

noinst_HEADERS = xmlstream.h xmlrpcd.h httpreq.h r2x.h session.h stateevents.h valueevents.h events.h plot.h altplot.h \
	valueplot.h emailaction.h altaz.h augerreq.h nightdur.h nightreq.h imgpreview.h obsreq.h targetreq.h dirsupport.h \
	devicesreq.h planreq.h graphreq.h bsc.h libjavascript.h libcss.h bbserver.h api.h bbapi.h images.h messageevents.h switchstatereq.h \
	directory.h expandstrings.h xmlapi.h jsonvalue.h asyncapi.h

AM_LDADD = @LIB_M@ @LIB_NOVA@ @JSONGLIB_LIBS@
AM_CXXFLAGS = @NOVA_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ @LIBARCHIVE_CFLAGS@ @JSONGLIB_CFLAGS@ -I../../lib/ -I../../include

if PGSQL

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp httpreq.cpp session.cpp events.cpp stateevents.cpp stateeventsdb.cpp valueevents.cpp \
	valueeventsdb.cpp emailaction.cpp plot.cpp altplot.cpp valueplot.cpp altaz.cpp augerreq.cpp nightdur.cpp \
	nightreq.cpp imgpreview.cpp obsreq.cpp targetreq.cpp dirsupport.cpp devicesreq.cpp planreq.cpp graphreq.cpp \
	libjavascript.cpp libcss.cpp bbserver.cpp api.cpp bbapi.cpp images.cpp messageevents.cpp switchstatereq.cpp directory.cpp \
	expandstrings.cpp xmlapi.cpp jsonvalue.cpp asyncapi.cpp
rts2_xmlrpcd_CXXFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ ${AM_CXXFLAGS}
rts2_xmlrpcd_LDADD= -L../../lib/rts2scheduler -lrts2scheduler -L../../lib/rts2script -lrts2script -L../../lib/rts2db -lrts2db \
	-L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 -L../../lib/xmlrpc++ -lrts2xmlrpc @LIBPG_LIBS@ \
	@LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@ @LIBXML_LIBS@ @LIB_CRYPT@ @LIBARCHIVE_LIBS@ $(AM_LDADD)

CLEANFILES = stateeventsdb.cpp valueeventsdb.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp httpreq.cpp session.cpp events.cpp stateevents.cpp valueevents.cpp emailaction.cpp \
	plot.cpp altaz.cpp nightdur.cpp imgpreview.cpp targetreq.cpp dirsupport.cpp devicesreq.cpp libjavascript.cpp \
	libcss.cpp bbserver.cpp api.cpp bbapi.cpp images.cpp messageevents.cpp switchstatereq.cpp directory.cpp expandstrings.cpp \
	xmlapi.cpp jsonvalue.cpp asyncapi.cpp
rts2_xmlrpcd_CXXFLAGS = @CFITSIO_CFLAGS@ ${AM_CXXFLAGS}
rts2_xmlrpcd_LDADD = -L../../lib/rts2script -lrts2script -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2users -lrts2 -L../../lib/xmlrpc++ -lrts2xmlrpc \
	@LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@ @LIBXML_LIBS@ @LIBARCHIVE_LIBS@ @LIB_CRYPT@ ${AM_LDADD}

endif

EXTRA_DIST = stateeventsdb.ec valueeventsdb.ec graphreq.cpp

rts2_xmlrpcclient_SOURCES = xmlrpcclient.cpp
rts2_xmlrpcclient_CXXFLAGS = @NOVA_CFLAGS@ ${AM_CXXFLAGS}
rts2_xmlrpcclient_LDADD = -L../../lib/rts2 -lrts2 -L../../lib/xmlrpc++ -lrts2xmlrpc @LIB_NOVA@ $(AM_LDADD)

rts2_jsonclient_SOURCES = jsonclient.cpp
rts2_jsonclient_CXXFLAGS = @NOVA_CFLAGS@ ${AM_CXXFLAGS}
rts2_jsonclient_LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@ $(AM_LDADD)
