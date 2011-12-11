noinst_HEADERS = rts2execcli.h script.h connimgprocess.h element.h devscript.h \
		rts2spiral.h elementblock.h elementguiding.h rts2execclidb.h elementacquire.h \
		rts2targetscr.h elementtarget.h elementhex.h elementwaitfor.h \
		rts2scriptinterface.h operands.h elementexe.h printtarget.h connexe.h connselector.h \
		executorque.h simulque.h


lib_LTLIBRARIES = librts2script.la

librts2script_la_SOURCES = rts2execcli.cpp script.cpp connimgprocess.cpp element.cpp devscript.cpp rts2spiral.cpp \
		elementblock.cpp elementguiding.cpp rts2targetscr.cpp elementtarget.cpp elementhex.cpp elementwaitfor.cpp \
		rts2scriptinterface.cpp operands.cpp elementexe.cpp connexe.cpp connselector.cpp
librts2script_la_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@

if PGSQL

librts2script_la_SOURCES += printtarget.cpp rts2execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp

else

EXTRA_DIST = printtarget.cpp rts2execclidb.cpp elementacquire.cpp executorque.cpp simulque.cpp

endif
