SUBDIRS = xmlrpc++

bin_PROGRAMS = rts2-xmlrpcd

rts2_xmlrpcd_SOURCES = xmlrpcd.cpp
rts2_xmlrpcd_CXXFLAGS = @LIBPG_CFLAGS@
rts2_xmlrpcd_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a xmlrpc++/libxmlrpc_rts2.a
