
#include "XmlRpcServerMethod.h"
#include "XmlRpcServer.h"

namespace XmlRpc
{

	XmlRpcServerMethod::XmlRpcServerMethod(std::string const& in_name, XmlRpcServer* server)
	{
		_name = in_name;
		_server = server;
		if (_server) _server->addMethod(this);
	}

	XmlRpcServerMethod::~XmlRpcServerMethod()
	{
		if (_server) _server->removeMethod(this);
	}

}								 // namespace XmlRpc
