SUBDIRS = simbad

if PGSQL

AM_CXXFLAGS = @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
AM_LDADD = @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@

bin_PROGRAMS = rts2-nightreport rts2-targetlist rts2-target rts2-tpm rts2-obsinfo rts2-user rts2-newtarget rts2-simbadinfo

noinst_HEADERS = rts2targetapp.h

rts2_nightreport_SOURCES = nightreport.cpp
rts2_nightreport_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDADD)

rts2_targetlist_SOURCES = targetlist.cpp
rts2_targetlist_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDADD)

rts2_target_SOURCES = target.cpp
rts2_target_LDADD = ../plan/libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDADD)

rts2_tpm_SOURCES = tpm.cpp
rts2_tpm_LDADD = ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDADD)

rts2_obsinfo_SOURCES = obsinfo.cpp
rts2_obsinfo_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDADD)

rts2_user_SOURCES = user.cpp
rts2_user_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDADD)

rts2_newtarget_SOURCES = newtarget.cpp rts2targetapp.cpp
rts2_newtarget_CXXFLAGS = @LIBXML_CFLAGS@ ${AM_CXXFLAGS}
rts2_newtarget_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a simbad/libsimbad.a ../../lib/xmlrpc++/librts2xmlrpc.a @LIBXML_LIBS@ $(AM_LDADD)

rts2_simbadinfo_SOURCES = simbadinfo.cpp rts2targetapp.cpp
rts2_simbadinfo_CXXFLAGS = @LIBXML_CFLAGS@ ${AM_CXXFLAGS}
rts2_simbadinfo_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a simbad/libsimbad.a ../../lib/xmlrpc++/librts2xmlrpc.a @LIBXML_LIBS@ $(AM_LDADD)

endif
