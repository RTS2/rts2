SUBDIRS = xmlrpc++

bin_PROGRAMS = rts2-xmlrpcd rts2-xmlrpcclient

noinst_HEADERS = xmlstream.h r2x.h session.h stateevents.h

AM_LDFLAGS = @LIB_M@ @LIB_NOVA@

if PGSQL

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp session.cpp stateevents.cpp
rts2_xmlrpcd_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@
rts2_xmlrpcd_LDFLAGS = ../scheduler/librts2scheduler.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a xmlrpc++/libxmlrpc_rts2.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@ @LIBXML_LIBS@ $(AM_LDFLAGS)

else

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp session.cpp stateevents.cpp
rts2_xmlrpcd_CXXFLAGS = @NOVA_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@
rts2_xmlrpcd_LDFLAGS = ../writers/librts2image.a ../utils/librts2block.a xmlrpc++/libxmlrpc_rts2.a @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@ @LIBXML_LIBS@ $(AM_LDFLAGS)

endif

rts2_xmlrpcclient_SOURCES = xmlrpcclient.cpp
rts2_xmlrpcclient_CXXFLAGS = @NOVA_CFLAGS@
rts2_xmlrpcclient_LDFLAGS = ../utils/librts2block.a xmlrpc++/libxmlrpc_rts2.a @LIB_NOVA@ $(AM_LDFLAGS)
