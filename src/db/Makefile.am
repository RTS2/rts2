# $Id$

#bin_PROGRAMS = rts2-photlist

#rts2_photlist_SOURCES = photlist.c
#rts2_photlist_LDFLAGS = @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ ../utils/config.o

AM_CXXFLAGS = @LIBPG_CFLAGS@
AM_CFLAGS = @LIBPG_CFLAGS@
AM_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@

bin_PROGRAMS = rts2-nightreport rts2-nightmails

rts2_nightreport_SOURCES = nightreport.cpp
rts2_nightreport_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_nightmails_SOURCES = nightmails.cpp
rts2_nightmails_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a
