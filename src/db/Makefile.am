SUBDIRS = simbad

if PGSQL

AM_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
AM_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@

bin_PROGRAMS = rts2-nightreport rts2-nightmails rts2-targetlist rts2-target rts2-tpm rts2-obsinfo rts2-user rts2-newtarget rts2-simbadinfo

noinst_HEADERS = rts2targetapp.h

rts2_nightreport_SOURCES = nightreport.cpp
rts2_nightreport_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDFLAGS)

rts2_nightmails_SOURCES = nightmails.cpp
rts2_nightmails_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDFLAGS)

rts2_targetlist_SOURCES = targetlist.cpp
rts2_targetlist_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDFLAGS)

rts2_target_SOURCES = target.cpp
rts2_target_LDFLAGS = ../plan/libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDFLAGS)

rts2_tpm_SOURCES = tpm.cpp
rts2_tpm_LDFLAGS = ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDFLAGS)

rts2_obsinfo_SOURCES = obsinfo.cpp
rts2_obsinfo_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDFLAGS)

rts2_user_SOURCES = user.cpp
rts2_user_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a $(AM_LDFLAGS)

rts2_newtarget_SOURCES = newtarget.cpp rts2targetapp.cpp
rts2_newtarget_CFLAGS = @LIBXML_CFLAGS@
rts2_newtarget_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a simbad/libsimbad.a ../xmlrpc/xmlrpc++/librts2xmlrpc.a @LIBXML_LIBS@ $(AM_LDFLAGS)

rts2_simbadinfo_SOURCES = simbadinfo.cpp rts2targetapp.cpp
rts2_simbadinfo_CFLAGS = @LIBXML_CFLAGS@
rts2_simbadinfo_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a simbad/libsimbad.a ../xmlrpc/xmlrpc++/librts2xmlrpc.a @LIBXML_LIBS@ $(AM_LDFLAGS)

endif
