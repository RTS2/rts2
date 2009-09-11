bin_PROGRAMS=

noinst_DATA = rts2.wsdl typemap.dat
noinst_HEADERS = rts2soapcli.h

EXTRA_DIST = rts2.wsdl stlvector.h typemap.dat

if GSOAP

bin_PROGRAMS += rts2-soapd
noinst_PROGRAMS = rts2-testsoap

SOAPHEADER = rts2.h
SOAP_CPP_CLIENT = soapClient.cpp soapC.cpp
SOAP_CPP_SERVER = soapServer.cpp

rts2.h: rts2.wsdl
	wsdl2h $< -o $@

SOAP_CPP_SRC = $(SOAP_CPP_CLIENT) $(SOAP_CPP_SERVER) $(SOAP_H_FILES) $(SOAP_NS)
$(SOAP_CPP_SRC):$(SOAPHEADER)
	@SOAP@ @SOAP_FLAGS@ $<

rts2_soapd_SOURCES = $(SOAP_CPP_CLIENT) $(SOAP_CPP_SERVER) soapd.cpp rts2soapcli.cpp
rts2_soapd_CXXFLAGS = @LIB_GSOAP_CFLAGS@ @LIBPG_CFLAGS@ @JPEG_CFLAGS@
rts2_soapd_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIB_GSOAP@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@
rts2_testsoap_SOURCES = $(SOAP_CPP_CLIENT) testsoap.cpp
rts2_testsoap_CXXFLAGS = @LIB_GSOAP_CFLAGS@
rts2_testsoap_LDFLAGS = ../utils/librts2block.a @LIB_GSOAP@

CLEANFILES = $(SOAP_CPP_SRC) *.req.xml *.res.xml *.nsmap *.xsd *Proxy\.h rts2.h

endif
