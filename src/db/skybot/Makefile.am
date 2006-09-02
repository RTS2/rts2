noinst_HEADERS = rts2skybottarget.h
noinst_DATA = skybot.wsdl typemap.dat

if GSOAP
noinst_LIBRARIES = libskybot.a

SOAPHEADER=skybot.h
SOAP_CPP_CLIENT = soapC.cpp soapClient.cpp
SOAP_CPP_SERVER = soapServerLib.cpp
SOAP_CPP_SRC=$(SOAP_CPP_CLIENT) $(SOAP_CPP_SERVER)

skybot.h: skybot.wsdl typemap.dat
	wsdl2h -s $< -o $@

soapC.cpp: skybot.h
	@SOAP@ $<

libskybot_a_SOURCES = $(SOAP_CPP_CLIENT) rts2skybottarget.cpp
libskybot_a_CXXFLAGS = @LIB_GSOAP_CFLAGS@

CLEANFILES = $(SOAP_CPP_SRC) *.req.xml *.res.xml *.nsmap *.xsd *Proxy\.h $(SOAPHEADER)

endif
