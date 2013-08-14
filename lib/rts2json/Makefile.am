lib_LTLIBRARIES = librts2json.la

librts2json_la_SOURCES = httpreq.cpp jsonvalue.cpp directory.cpp expandstrings.cpp libjavascript.cpp \
	images.cpp targetreq.cpp altaz.cpp plot.cpp imgpreview.cpp nightdur.cpp asyncapi.cpp httpserver.cpp \
	libcss.cpp
librts2json_la_CXXFLAGS = -I../../include @LIBXML_CFLAGS@ -I../ @JPEG_CFLAGS@ @NOVA_CFLAGS@

noinst_SCRIPTS = images_convert

if PGSQL

librts2json_la_SOURCES += jsondb.cpp altplot.cpp nightreq.cpp obsreq.cpp addtargetreq.cpp

else

EXTRA_DIST = jsondb.cpp altplot.cpp nightreq.cpp obsreq.cpp addtargetreq.cpp

endif
