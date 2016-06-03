noinst_HEADERS = elementhex.h elementwaitfor.h elementexe.h

lib_LTLIBRARIES = librts2script.la

librts2script_la_SOURCES = execcli.cpp script.cpp connimgprocess.cpp element.cpp devscript.cpp rts2spiral.cpp \
		elementblock.cpp scripttarget.cpp elementtarget.cpp elementhex.cpp elementwaitfor.cpp \
		scriptinterface.cpp operands.cpp elementexe.cpp connexe.cpp connselector.cpp
librts2script_la_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ @LIBXML_CFLAGS@ -I../../include

if PGSQL

librts2script_la_SOURCES += printtarget.cpp execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp

else

EXTRA_DIST = printtarget.cpp execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp

endif
