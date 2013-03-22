lib_LTLIBRARIES = librts2json.la

librts2json_la_SOURCES = httpreq.cpp jsonvalue.cpp directory.cpp expandstrings.cpp libjavascript.cpp
librts2json_la_CXXFLAGS = -I../../include @LIBXML_CFLAGS@ -I../ -I../xmlrpc++ @JPEG_CFLAGS@

if PGSQL

librts2json_la_SOURCES += jsondb.cpp

else

EXTRA_DIST = jsondb.cpp

endif
