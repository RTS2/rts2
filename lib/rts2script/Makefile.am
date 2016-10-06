noinst_HEADERS = elementhex.h elementwaitfor.h elementexe.h

lib_LTLIBRARIES = librts2script.la

librts2script_la_SOURCES = execcli.cpp script.cpp connimgprocess.cpp element.cpp devscript.cpp rts2spiral.cpp \
		elementblock.cpp scripttarget.cpp elementtarget.cpp elementhex.cpp elementwaitfor.cpp \
		scriptinterface.cpp operands.cpp elementexe.cpp connexe.cpp connselector.cpp
librts2script_la_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ @LIBXML_CFLAGS@ -I../../include

if PGSQL

librts2script_la_SOURCES += printtarget.cpp execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp
librts2script_la_LIBADD = ../rts2db/librts2db.la ../rts2fits/librts2imagedb.la

else

librts2script_la_LIBADD = ../rts2/librts2.la ../rts2fits/librts2image.la

EXTRA_DIST = printtarget.cpp execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp

endif
