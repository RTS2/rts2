# $iD: mAKEFILE.AM,v 1.3.4.16 2007-07-29 20:27:57 petr Exp $

bin_PROGRAMS = rts2-mon rts2-cmon rts2-talker

noinst_HEADERS = nmonitor.h rts2nwindow.h rts2daemonwindow.h rts2nmenu.h rts2nmsgbox.h rts2nmsgwindow.h rts2nstatuswindow.h \
	rts2ncomwin.h rts2nlayout.h rts2nvaluebox.h rts2ndevicewindow.h rts2nwindowedit.h

rts2_mon_SOURCES = nmonitor.cpp rts2nwindow.cpp rts2daemonwindow.cpp rts2nmenu.cpp rts2nmsgbox.cpp rts2nmsgwindow.cpp \
	rts2nstatuswindow.cpp rts2ncomwin.cpp rts2nlayout.cpp rts2nvaluebox.cpp rts2ndevicewindow.cpp rts2nwindowedit.cpp

if PGSQL

rts2_mon_LDFLAGS = @LIBNCURSES_LDFLAGS@ @LIB_M@ @LIB_NOVA@ @LIBXML_LIBS@ @LIB_ECPG@ ../db/simbad/libsimbad.a ../../lib/xmlrpc++/librts2xmlrpc.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIB_CFITSIO@ @LIB_JPEG@
rts2_mon_CXXFLAGS = @LIBNCURSES_CFLAGS@ @NOVA_CFLAGS@ @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

else

rts2_mon_LDFLAGS = @LIBNCURSES_LDFLAGS@ @LIB_M@ @LIB_NOVA@ ../utils/librts2block.a @LIB_CFITSIO@ @LIB_JPEG@
rts2_mon_CXXFLAGS = @LIBNCURSES_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

endif

rts2_cmon_SOURCES = cmonitor.cpp
rts2_cmon_LDFLAGS = @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a
rts2_cmon_CXXFLAGS = @NOVA_CFLAGS@

rts2_talker_SOURCES = talker.cpp
rts2_talker_LDFLAGS = @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a
rts2_talker_CXXFLAGS = @NOVA_CFLAGS@
