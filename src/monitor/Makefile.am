# $iD: mAKEFILE.AM,v 1.3.4.16 2007-07-29 20:27:57 petr Exp $

bin_PROGRAMS = rts2-mon rts2-cmon rts2-talker

noinst_HEADERS = nmonitor.h nwindow.h daemonwindow.h nmenu.h nmsgbox.h nmsgwindow.h nstatuswindow.h \
	ncomwin.h nlayout.h nvaluebox.h ndevicewindow.h nwindowedit.h

rts2_mon_SOURCES = nmonitor.cpp nwindow.cpp daemonwindow.cpp nmenu.cpp nmsgbox.cpp nmsgwindow.cpp \
	nstatuswindow.cpp ncomwin.cpp nlayout.cpp nvaluebox.cpp ndevicewindow.cpp nwindowedit.cpp

if PGSQL

rts2_mon_CXXFLAGS = @LIBXML_CFLAGS@ @NCURSESW_CFLAGS@ @NOVA_CFLAGS@ @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ @JSONGLIB_CFLAGS@ -I../../include
rts2_mon_LDADD = -L../../lib/rts2db -lrts2db -L../../lib/pluto -lpluto -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @CFITSIO_LIBS@ @MAGIC_LIBS@ @NCURSESW_LIBS@ @LIB_M@ @LIB_CRYPT@ @LIB_NOVA@ @LIB_ECPG@

else

rts2_mon_CXXFLAGS = @NCURSESW_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ -I../../include
rts2_mon_LDADD = -L../../lib/rts2 -lrts2 @CFITSIO_LIBS@ @MAGIC_LIBS@ @NCURSESW_LIBS@ @LIB_M@ @LIB_NOVA@

endif

rts2_cmon_SOURCES = cmonitor.cpp
rts2_cmon_CXXFLAGS = @NOVA_CFLAGS@ -I../../include
rts2_cmon_LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@ @LIB_M@ 

rts2_talker_SOURCES = talker.cpp
rts2_talker_CXXFLAGS = @NOVA_CFLAGS@ -I../../include
rts2_talker_LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@ @LIB_M@ 
