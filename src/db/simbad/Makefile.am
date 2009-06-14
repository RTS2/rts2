noinst_HEADERS = rts2simbadtarget.h
noinst_DATA = simbad.wsdl typemap.dat

EXTRA_DIST = simbad.wsdl typemap.dat

if GSOAP
noinst_LIBRARIES = libsimbad.a

SOAPHEADER=simbad.h
SOAP_CPP_CLIENT = soapC.cpp soapClient.cpp
SOAP_CPP_SERVER = soapServerLib.cpp
SOAP_CPP_SRC=$(SOAP_CPP_CLIENT) $(SOAP_CPP_SERVER)

simbad.h: simbad.wsdl typemap.dat
	wsdl2h -s $< -o $@

soapC.cpp: simbad.h
	@SOAP@ $<

libsimbad_a_SOURCES = $(SOAP_CPP_CLIENT) rts2simbadtarget.cpp
libsimbad_a_CXXFLAGS = @LIB_GSOAP_CFLAGS@

CLEANFILES = $(SOAP_CPP_SRC) *.req.xml *.res.xml *.nsmap *.xsd *Proxy\.h $(SOAPHEADER)

else

EXTRA_DIST += simbad.h rts2simbadtarget.cpp

endif
