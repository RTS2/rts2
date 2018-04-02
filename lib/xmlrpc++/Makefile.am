lib_LTLIBRARIES = librts2xmlrpc.la

librts2xmlrpc_la_SOURCES = \
	urlencoding.cpp \
	XmlRpcClient.cpp \
	XmlRpcDispatch.cpp \
	XmlRpcServerConnection.cpp \
	XmlRpcServer.cpp \
	XmlRpcServerMethod.cpp \
	XmlRpcServerGetRequest.cpp \
	XmlRpcSource.cpp \
	XmlRpcUtil.cpp \
	XmlRpcValue.cpp \
	XmlRpcSocket.cpp

librts2xmlrpc_la_CXXFLAGS = @NOVA_CFLAGS@ -I../../include -I../../include/xmlrpc++

if MACOSX
librts2xmlrpc_la_CXXFLAGS += -include ../../include/compat/osx/compat.h
endif

if SSL

librts2xmlrpc_la_SOURCES += XmlRpcSocketSSL.cpp
librts2xmlrpc_la_LIBADD = @SSL_LIBS@

else

EXTRA_DIST = XmlRpcSocketSSL.cpp

endif
