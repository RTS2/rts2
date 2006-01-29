# $Id$

AM_CXXFLAGS = @LIBPG_CFLAGS@
AM_CFLAGS = @LIBPG_CFLAGS@
AM_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@

bin_PROGRAMS = rts2-nightreport rts2-nightmails

noinst_HEADERS = rts2simbadtarget.h simbad.h

rts2_nightreport_SOURCES = nightreport.cpp
rts2_nightreport_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_nightmails_SOURCES = nightmails.cpp
rts2_nightmails_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

if GSOAP

bin_PROGRAMS+=rts2-newtarget

SOAPHEADER=simbad.h
SOAP_CPP_CLIENT=soapClient.cpp soapC.cpp

SOAP_CPP_SRC=$(SOAP_CPP_CLIENT)
$(SOAP_CPP_SRC) : $(SOAPHEADER)
	@SOAP@ @SOAP_FLAGS@ $<
  
rts2_newtarget_SOURCES = newtarget.cpp $(SOAP_CPP_CLIENT) rts2simbadtarget.cpp
rts2_newtarget_CXXFLAGS = @LIBPG_CFLAGS@ @LIB_GSOAP_CFLAGS@
rts2_newtarget_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_GSOAP@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a
endif
