
#include "XmlRpcServer.h"
#include "XmlRpcServerConnection.h"
#include "XmlRpcServerMethod.h"
#include "XmlRpcServerGetRequest.h"
#include "XmlRpcUtil.h"
#include "XmlRpcException.h"

extern "C" {
	# include <arpa/inet.h>
}

using namespace XmlRpc;

#ifdef RTS2_SSL
int XmlRpcServer::initSSL(const char *certFile, const char *keyFile)
{
	return XmlRpcSocketSSL::initSSL(certFile, keyFile);
}
#endif

XmlRpcServer::XmlRpcServer()
{
	_introspectionEnabled = false;
	_listMethods = NULL;
	_methodHelp = NULL;
	_defaultGetRequest = NULL;
}


XmlRpcServer::~XmlRpcServer()
{
	this->shutdown();
	_methods.clear();
	_requests.clear();
	delete _listMethods;
	delete _methodHelp;
	delete _defaultGetRequest;
}

// Add a command to the RPC server
void XmlRpcServer::addMethod(XmlRpcServerMethod* method)
{
	_methods[method->name()] = method;
}

// Remove a command from the RPC server
void XmlRpcServer::removeMethod(XmlRpcServerMethod* method)
{
	MethodMap::iterator i = _methods.find(method->name());
	if (i != _methods.end())
		_methods.erase(i);
}

// Remove a command from the RPC server by name
void XmlRpcServer::removeMethod(const std::string& methodName)
{
	MethodMap::iterator i = _methods.find(methodName);
	if (i != _methods.end())
		_methods.erase(i);
}

// Look up a method by name
XmlRpcServerMethod* XmlRpcServer::findMethod(const std::string& name) const
{
	MethodMap::const_iterator i = _methods.find(name);
	if (i == _methods.end())
		return 0;
	return i->second;
}

void XmlRpcServer::setDefaultGetRequest(XmlRpcServerGetRequest *defaultGetRequest)
{
	_defaultGetRequest = defaultGetRequest;
}

// Add a GET request to the HTTP server
void XmlRpcServer::addGetRequest(XmlRpcServerGetRequest* serverGetRequest)
{
	_requests[serverGetRequest->getPrefix()] = serverGetRequest;
}

// Remove a GET request from HTTP server
void XmlRpcServer::removeGetRequest(XmlRpcServerGetRequest* serverGetRequest)
{
	RequestMap::iterator i = _requests.find(serverGetRequest->getPrefix());
	if (i != _requests.end())
		_requests.erase(i);
}

// Lookup a get request by prefix
XmlRpcServerGetRequest*XmlRpcServer::findGetRequest(const std::string& prefix) const
{
	for (RequestMap::const_iterator i = _requests.begin(); i != _requests.end(); i++)
	{
		if (prefix.find(i->first) == 0)
			return i->second;
	}
	return _defaultGetRequest;
}

// Create a socket, bind to the specified port, and
// set it in listen mode to make it available for clients.
bool XmlRpcServer::bindAndListen(int port, int backlog /*= 5*/)
{
	int fd = XmlRpcSocket::socket();
	if (fd < 0)
	{
		XmlRpcUtil::error("XmlRpcServer::bindAndListen: Could not create socket (%s).", XmlRpcSocket::getErrorMsg().c_str());
		return false;
	}

	this->setfd(fd);

	// Don't block on reads/writes
	if ( ! XmlRpcSocket::setNonBlocking(fd))
	{
		this->close();
		XmlRpcUtil::error("XmlRpcServer::bindAndListen: Could not set socket to non-blocking input mode (%s).", XmlRpcSocket::getErrorMsg().c_str());
		return false;
	}

	// Allow this port to be re-bound immediately so server re-starts are not delayed
	if ( ! XmlRpcSocket::setReuseAddr(fd))
	{
		this->close();
		XmlRpcUtil::error("XmlRpcServer::bindAndListen: Could not set SO_REUSEADDR socket option (%s).", XmlRpcSocket::getErrorMsg().c_str());
		return false;
	}

	// Bind to the specified port on the default interface
	if ( ! XmlRpcSocket::bind(fd, port))
	{
		this->close();
		XmlRpcUtil::error("XmlRpcServer::bindAndListen: Could not bind to specified port (%s).", XmlRpcSocket::getErrorMsg().c_str());
		return false;
	}

	// Set in listening mode
	if ( ! XmlRpcSocket::listen(fd, backlog))
	{
		this->close();
		XmlRpcUtil::error("XmlRpcServer::bindAndListen: Could not set socket in listening mode (%s).", XmlRpcSocket::getErrorMsg().c_str());
		return false;
	}

	XmlRpcUtil::log(2, "XmlRpcServer::bindAndListen: server listening on port %d fd %d", port, fd);

	// Notify the dispatcher to listen on this source when we are in work()
	_disp.addSource(this, XmlRpcDispatch::ReadableEvent);

	return true;
}

//! Modify the types of events to watch for on this source
void XmlRpcServer::setSourceEvents(XmlRpcSource* source, unsigned eventMask)
{
	_disp.setSourceEvents(source, eventMask);
}

// Process client requests for the specified time
void XmlRpcServer::work(double msTime)
{
	XmlRpcUtil::log(2, "XmlRpcServer::work: waiting for a connection");
	_disp.work(msTime);
}

// Add sockets to file descriptor set
void XmlRpcServer::addToFd (fd_set *inFd, fd_set *outFd, fd_set *excFd)
{
	_disp.addToFd(inFd, outFd, excFd);
}

// Check if it should server any of the modified sockets
void XmlRpcServer::checkFd (fd_set *inFd, fd_set *outFd, fd_set *excFd)
{
	_disp.checkFd(inFd, outFd, excFd);
}

// Handle input on the server socket by accepting the connection
// and reading the rpc request.
unsigned XmlRpcServer::handleEvent(unsigned mask)
{
	acceptConnection();
								 // Continue to monitor this fd
	return XmlRpcDispatch::ReadableEvent;
}

// Accept a client connection request and create a connection to
// handle method calls from the client.
void XmlRpcServer::acceptConnection()
{
	struct sockaddr_in saddr;
#ifdef _WINDOWS
	int addrlen;
#else
	socklen_t addrlen;
#endif
	int s = XmlRpcSocket::accept(this->getfd(), saddr, addrlen);
	XmlRpcUtil::log(2, "XmlRpcServer::acceptConnection: socket %d from %s", s, inet_ntoa (saddr.sin_addr));
	if (s < 0)
	{
		//this->close();
		XmlRpcUtil::error("XmlRpcServer::acceptConnection: Could not accept connection (%s).", XmlRpcSocket::getErrorMsg().c_str());
	}
	else if ( ! XmlRpcSocket::setNonBlocking(s))
	{
		XmlRpcSocket::close(s);
		XmlRpcUtil::error("XmlRpcServer::acceptConnection: Could not set socket to non-blocking input mode (%s).", XmlRpcSocket::getErrorMsg().c_str());
	}
	else						 // Notify the dispatcher to listen for input on this source when we are in work()
	{
		XmlRpcUtil::log(2, "XmlRpcServer::acceptConnection: creating a connection");
		_disp.addSource(this->createConnection(s, &saddr, addrlen), XmlRpcDispatch::ReadableEvent);
	}
}

// Create a new connection object for processing requests from a specific client.
#ifdef _WINDOWS
XmlRpcServerConnection* XmlRpcServer::createConnection(int s, struct sockaddr_in *saddr, int addrlen)
#else
XmlRpcServerConnection* XmlRpcServer::createConnection(int s, struct sockaddr_in *saddr, socklen_t addrlen)
#endif
{
	// Specify that the connection object be deleted when it is closed
	return new XmlRpcServerConnection(s, this, true, saddr, addrlen);
}

void XmlRpcServer::removeConnection(XmlRpcServerConnection* sc)
{
	_disp.removeSource(sc);
}

// Stop processing client requests
void XmlRpcServer::exitWork()
{
	_disp.exitWork();
}

// Close the server socket file descriptor and stop monitoring connections
void XmlRpcServer::shutdown()
{
	// This closes and destroys all connections as well as closing this socket
	_disp.clear();
}

// Introspection support
static const std::string LIST_METHODS("system.listMethods");
static const std::string METHOD_HELP("system.methodHelp");
static const std::string MULTICALL("system.multicall");

// List all methods available on a server
class ListMethods : public XmlRpcServerMethod
{
	public:
		ListMethods(XmlRpcServer* s) : XmlRpcServerMethod(LIST_METHODS, s) {}

		void execute(struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result)
		{
			_server->listMethods(result);
		}

		std::string help() { return std::string("List all methods available on a server as an array of strings"); }
};

// Retrieve the help string for a named method
class MethodHelp : public XmlRpcServerMethod
{
	public:
		MethodHelp(XmlRpcServer* s) : XmlRpcServerMethod(METHOD_HELP, s) {}

		void execute(struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result)
		{
			if (params[0].getType() != XmlRpcValue::TypeString)
				throw XmlRpcException(METHOD_HELP + ": Invalid argument type");

			XmlRpcServerMethod* m = _server->findMethod(params[0]);
			if ( ! m)
				throw XmlRpcException(METHOD_HELP + ": Unknown method name");

			result = m->help();
		}

		std::string help() { return std::string("Retrieve the help string for a named method"); }
};

// Specify whether introspection is enabled or not. Default is enabled.
void XmlRpcServer::enableIntrospection(bool enabled)
{
	if (_introspectionEnabled == enabled)
		return;

	_introspectionEnabled = enabled;

	if (enabled)
	{
		if ( ! _listMethods)
		{
			_listMethods = new ListMethods(this);
			_methodHelp = new MethodHelp(this);
		}
		else
		{
			addMethod(_listMethods);
			addMethod(_methodHelp);
		}
	}
	else
	{
		removeMethod(LIST_METHODS);
		removeMethod(METHOD_HELP);
	}
}

void XmlRpcServer::listMethods(XmlRpcValue& result)
{
	int i = 0;
	result.setSize(_methods.size()+1);
	for (MethodMap::iterator it=_methods.begin(); it != _methods.end(); ++it)
		result[i++] = it->first;

	// Multicall support is built into XmlRpcServerConnection
	result[i] = MULTICALL;
}
