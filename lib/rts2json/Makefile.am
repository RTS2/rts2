lib_LTLIBRARIES = librts2json.la

librts2json_la_SOURCES = httpreq.cpp jsonvalue.cpp directory.cpp expandstrings.cpp libjavascript.cpp \
	images.cpp targetreq.cpp altaz.cpp plot.cpp imgpreview.cpp nightdur.cpp asyncapi.cpp httpserver.cpp \
	libcss.cpp
librts2json_la_CXXFLAGS = -I../../include @LIBXML_CFLAGS@ -I../ @MAGIC_CFLAGS@ @CFITSIO_CFLAGS@ @NOVA_CFLAGS@
librts2json_la_LIBADD = ../rts2/librts2.la @LIBARCHIVE_LIBS@

noinst_SCRIPTS = images_convert

EXTRA_DIST = images_convert

if PGSQL

librts2json_la_SOURCES += jsondb.cpp altplot.cpp nightreq.cpp obsreq.cpp addtargetreq.cpp
librts2json_la_LIBADD += ../rts2db/librts2db.la

else

EXTRA_DIST += jsondb.cpp altplot.cpp nightreq.cpp obsreq.cpp addtargetreq.cpp

endif
