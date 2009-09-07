
#include "XmlRpcServerGetRequest.h"
#include "XmlRpcServer.h"

#include "string.h"

namespace XmlRpc
{

	XmlRpcServerGetRequest::XmlRpcServerGetRequest(std::string const& in_prefix, XmlRpcServer* server)
	{
		_prefix = in_prefix;
		_server = server;
		if (_server) _server->addGetRequest(this);
	}

	XmlRpcServerGetRequest::~XmlRpcServerGetRequest()
	{
		if (_server) _server->removeGetRequest(this);
	}

	void XmlRpcServerGetRequest::setAuthorization(std::string authorization)
	{
	 	if (authorization.length () == 0)
		{
			_username = "";
			_password = "";
			return;
		}
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

	void XmlRpcServerGetRequest::authorizePage(int &http_code, const char* &response_type, char* &response, int &response_length)
	{
		http_code = HTTP_UNAUTHORIZED;
		response_type = "text/html";

		const char *r = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd\"><HTML><HEAD><TITLE>Error</TITLE><META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=ISO-8859-1\"></HEAD><BODY><H1>401 Unauthorised.</H1></BODY></HTML>";

		response_length = strlen (r);

		response = new char[response_length + 1];
		strcpy (response, r);
	}
}								 // namespace XmlRpc
