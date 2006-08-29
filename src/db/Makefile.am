# $Id$

AM_CXXFLAGS = @LIBPG_CFLAGS@
AM_CFLAGS = @LIBPG_CFLAGS@
AM_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@

bin_PROGRAMS = rts2-nightreport rts2-nightmails rts2-targetlist rts2-target rts2-tpm rts2-obsinfo

noinst_HEADERS = rts2targetapp.h rts2simbadtarget.h

rts2_nightreport_SOURCES = nightreport.cpp
rts2_nightreport_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_nightmails_SOURCES = nightmails.cpp
rts2_nightmails_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_targetlist_SOURCES = targetlist.cpp
rts2_targetlist_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_target_SOURCES = target.cpp
rts2_target_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_tpm_SOURCES = tpm.cpp
rts2_tpm_LDFLAGS = @LIB_NOVA@ @LIB_CFITSIO@ ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_obsinfo_SOURCES = obsinfo.cpp
rts2_obsinfo_LDFLAGS = @LIB_NOVA@ @LIB_CFITSIO@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

noinst_DATA = sesame.wsdl skybot.wsdl

if GSOAP
bin_PROGRAMS+=rts2-newtarget rts2-simbadinfo

SOAPHEADER=sesame.h skybot.h
SOAP_CPP_CLIENT = soapClient.cpp soapC.cpp
SOAP_CPP_SERVER = soapServer.cpp


sesame.h: sesame.wsdl
	wsdl2h -s $< -o $@
	@SOAP@ -i $@

skybot.h: skybot.wsdl
	wsdl2h -s $< -o $@
	@SOAP@ -i $@

SOAP_CPP_SRC=$(SOAP_CPP_CLIENT) $(SOAP_CPP_SERVER) 
$(SOAP_CPP_SRC) : $(SOAPHEADER)
	@SOAP@ @SOAP_FLAGS@ $<
  
rts2_newtarget_SOURCES = newtarget.cpp rts2targetapp.cpp $(SOAP_CPP_CLIENT) rts2simbadtarget.cpp
rts2_newtarget_CXXFLAGS = @LIBPG_CFLAGS@ @LIB_GSOAP_CFLAGS@
rts2_newtarget_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_GSOAP@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_simbadinfo_SOURCES = simbadinfo.cpp rts2targetapp.cpp $(SOAP_CPP_CLIENT) rts2simbadtarget.cpp
rts2_simbadinfo_CXXFLAGS = @LIBPG_CFLAGS@ @LIB_GSOAP_CFLAGS@
rts2_simbadinfo_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_GSOAP@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

CLEANFILES = $(SOAP_CPP_SRC) *.req.xml *.res.xml *.nsmap *.xsd *Proxy\.h sesame.h skybot.h

endif
