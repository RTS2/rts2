SUBDIRS = xmlrpc++

bin_PROGRAMS = rts2-xmlrpcclient

noinst_HEADERS = xmlstream.h r2x.h

AM_LDFLAGS = @LIB_M@ @LIB_NOVA@

if PGSQL

bin_PROGRAMS += rts2-xmlrpcd 

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp
rts2_xmlrpcd_CXXFLAGS = @LIBPG_CFLAGS@
rts2_xmlrpcd_LDFLAGS = ../scheduler/librts2scheduler.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a xmlrpc++/libxmlrpc_rts2.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ $(AM_LDFLAGS)

else

EXTRA_DIST = xmlrpcd.cpp

endif

rts2_xmlrpcclient_SOURCES = xmlrpcclient.cpp
rts2_xmlrpcclient_LDFLAGS = ../utils/librts2block.a xmlrpc++/libxmlrpc_rts2.a $(AM_LDFLAGS)

