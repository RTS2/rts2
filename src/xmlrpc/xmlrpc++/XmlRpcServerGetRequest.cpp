
#include "XmlRpcServerGetRequest.h"
#include "XmlRpcServer.h"

#include "string.h"
#include <stdlib.h>
#include <algorithm>

namespace XmlRpc
{
	const char *HttpParams::getString (const char *_name, const char *def_val)
	{
		for (HttpParams::iterator p = begin (); p != end (); p++)
		{
			if (p->haveName (_name))
				return p->getValue ();
		}
		return def_val;
	}

	int HttpParams::getInteger (const char *_name, int def_val)
	{
		const char *v = getString (_name, NULL);
		char *err;
		if (v == NULL)
			return def_val;
		int r = strtol (v, &err, 0);
		if (*v != '\0' && *err == '\0')
			return r;
		return def_val;
	}

	void HttpParams::parseParam (const std::string& ps)
	{
		// find = to split param and value
		std::string::size_type pv = ps.find ('=');
		if (pv != std::string::npos)
			addParam (ps.substr (0, pv).c_str (), ps.substr (pv + 1).c_str ());
		else
			addParam (ps.c_str (), "");
	}

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
