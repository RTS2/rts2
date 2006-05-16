bin_PROGRAMS=

if GSOAP

bin_PROGRAMS += rts2-soapd
noinst_DATA = rts2.wsdl

SOAPHEADER = rts2.h
SOAP_CPP_CLIENT = soapClient.cpp soapC.cpp
SOAP_CPP_SERVER = soapServer.cpp

SOAP_CPP_SRC = $(SOAP_CPP_CLIENT) $(SOAP_CPP_SERVER) $(SOAP_H_FILES) $(SOAP_NS)
$(SOAP_CPP_SRC):$(SOAPHEADER)
	@SOAP@ @SOAP_FLAGS@ $<

rts2_soapd_SOURCES = soapd.cpp $(SOAPHEADER) $(SOAP_CPP_CLIENT) $(SOAP_CPP_SERVER)
rts2_soapd_CXXFLAGS = @LIB_GSOAP_CFLAGS@
rts2_soapd_LDFLAGS = @LIB_GSOAP@

CLEANFILES = $(SOAP_CPP_SRC) *.req.xml *.res.xml *.nsmap *.xsd *Proxy\.h

endif
