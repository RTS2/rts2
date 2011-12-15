noinst_HEADERS = execcli.h script.h connimgprocess.h element.h devscript.h \
		rts2spiral.h elementblock.h elementguiding.h execclidb.h elementacquire.h \
		scripttarget.h elementtarget.h elementhex.h elementwaitfor.h \
		scriptinterface.h operands.h elementexe.h printtarget.h connexe.h connselector.h \
		executorque.h simulque.h


lib_LTLIBRARIES = librts2script.la

librts2script_la_SOURCES = execcli.cpp script.cpp connimgprocess.cpp element.cpp devscript.cpp rts2spiral.cpp \
		elementblock.cpp elementguiding.cpp scripttarget.cpp elementtarget.cpp elementhex.cpp elementwaitfor.cpp \
		scriptinterface.cpp operands.cpp elementexe.cpp connexe.cpp connselector.cpp
librts2script_la_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@

if PGSQL

librts2script_la_SOURCES += printtarget.cpp execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp

else

EXTRA_DIST = printtarget.cpp execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp

endif
