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
	XmlRpcValue.cpp

librts2xmlrpc_la_CXXFLAGS = @NOVA_CFLAGS@ -I../../include -I../../include/xmlrpc++

if SSL

librts2xmlrpc_la_SOURCES += XmlRpcSocketSSL.cpp
librts2xmlrpc_la_LIBADD = @SSL_LIBS@
EXTRA_DIST = XmlRpcSocket.cpp

else

librts2xmlrpc_la_SOURCES += XmlRpcSocket.cpp
EXTRA_DIST = XmlRpcSocketSSL.cpp

endif
