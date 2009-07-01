
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

	void XmlRpcServerMethod::setAuthentification(std::string authentification)
	{
		size_t dp = authentification.find(':');
		if (dp != std::string::npos)
		{
			_username = authentification.substr(0,dp);
			_password = authentification.substr(dp+1);
		}
		else
		{
			_username = authentification;
			_password = "";
		}
	}

}								 // namespace XmlRpc
