lib_LTLIBRARIES = librts2xmlrpc.la

noinst_HEADERS = \
	base64.h \
	urlencoding.h \
	XmlRpcClient.h \
	XmlRpcDispatch.h \
	XmlRpcException.h \
	XmlRpc.h \
	XmlRpcServerConnection.h \
	XmlRpcServer.h \
	XmlRpcServerMethod.h \
	XmlRpcServerGetRequest.h \
	XmlRpcSocket.h \
	XmlRpcSource.h \
	XmlRpcUtil.h \
	XmlRpcValue.h

librts2xmlrpc_la_SOURCES = \
	urlencoding.cpp \
	XmlRpcClient.cpp \
	XmlRpcDispatch.cpp \
	XmlRpcServerConnection.cpp \
	XmlRpcServer.cpp \
	XmlRpcServerMethod.cpp \
	XmlRpcServerGetRequest.cpp \
	XmlRpcSocket.cpp \
	XmlRpcSource.cpp \
	XmlRpcUtil.cpp \
	XmlRpcValue.cpp

librts2xmlrpc_la_CXXFLAGS = @NOVA_CFLAGS@ -I../../include
