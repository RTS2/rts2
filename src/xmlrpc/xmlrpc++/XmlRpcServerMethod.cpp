
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

	void XmlRpcServerMethod::setAuthorization(std::string authorization)
	{
		size_t dp = authorization.find(':');
		if (dp != std::string::npos)
		{
			_username = authorization.substr(0,dp);
			_password = authorization.substr(dp+1);
		}
		else
		{
			_username = authorization;
			_password = "";
		}
	}

}								 // namespace XmlRpc
