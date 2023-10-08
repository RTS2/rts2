#ifndef _XMLRPCSERVERGETREQUEST_H_
#define _XMLRPCSERVERGETREQUEST_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//          Copyright (c) 2009-2010 by Petr Kubanek
//          Copyright (c) 2011 by Petr Kubanek, Institute of Physics
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)	 // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <string>
# include <vector>
#endif

#include <sstream>

#include "XmlRpcServerConnection.h"

#define HTTP_OK              200
#define HTTP_BAD_REQUEST     400
#define HTTP_UNAUTHORIZED    401

namespace XmlRpc
{
	// The XmlRpcServer processes client requests to call GET methods.
	class XmlRpcServer;

	/**
	 * Parameter passed on GET request. Holds string representation of a
	 * parameter and its name.
	 */
	class HttpParam
	{
		public:
			HttpParam (std::string _name, std::string _val) { name = _name; val = _val; }
			const char *getName () { return name.c_str (); };
			const char *getValue () { return val.c_str (); };

			bool haveName (const char *_name) { return std::string (_name) == name; }
		private:
			std::string name;
			std::string val;
	};

	/**
	 * Class representing all parameters passed to GET call. Provides
	 * methods for access to parameter values.
	 */
	class HttpParams:public std::vector <HttpParam>
	{
		public:
			HttpParams () {};
			void addParam (std::string _name, std::string _val) { push_back (HttpParam (_name, _val)); }

			bool hasParam (const char *_name);

			std::string getValue (const char *_name, const char *dev_val);
			const char *getString (const char *_name, const char *def_val);
			int getInteger (const char *_name, int def_val);
			long getLong (const char *_name, long def_val);

			/**
			 * Return parameter as boolean.
			 * @param _name    argument name
			 * @param def_val  default value
			 *
			 * @return Returns true if parameter is 1, T or t. Return false if parameter is 0, F or f. Otherwise returns default value.
			 */
			bool getBoolean (const char *_name, bool def_val);
			double getDouble (const char *_name, double def_val);

			/**
			 * Return date as double ctime.
			 *
			 * @param _name  name of variable which will be returned.
			 * @param def_val default value. You can use time(NULL) to use current time.
			 * @param next_midnight  if only date is specified, return next local midnight time
			 *
			 * @return
			 */
			double getDate (const char *_name, double def_val, bool next_midnight = false);

			void parseParam (const std::string& ps);
			void parse (const std::string &ps);
	};

	//! Abstract class representing a single GET request
	class XmlRpcServerGetRequest
	{
		public:
			//! Constructor
			XmlRpcServerGetRequest (const char *prefix, const char *description = NULL, XmlRpcServer* server = 0);
			//! Destructor
			virtual ~XmlRpcServerGetRequest ();

			//! Returns the name of the method
			std::string& getPrefix () { return _prefix; }

			const char* getDescription () { return _description; }

			//! Set authentification string..
			void setAuthorization (std::string authorization);

			//! Set request base (per-request page prefix) for this request
			void setRequestBase (std::string prefix) { _request_base = prefix; }

			//! Return request base (per-request page prefix)
			std::string getRequestBase () { return _request_base; }

			//! Return username
			std::string getUsername () { return _username; }

			//! Return user password
			std::string getPassword () { return _password; }

			//! Execute the method. Subclasses must provide a definition for this method.
			virtual void execute (XmlRpcSource *source, sockaddr_in *saddr, std::string path, HttpParams *params, int &http_code, const char* &response_type, char* &respose, size_t &response_length) = 0;
			//! Returns 401 page
			virtual void authorizePage (int &http_code, const char* &response_type, char* &response, size_t &response_length);

			void setConnection (XmlRpcServerConnection *_connection) { connection = _connection; }

			//! Send JSON to XmlRpcSource connection. Re-enables read mask (as async call finished)
			void sendAsyncJSON (std::ostringstream &_os, XmlRpcServerConnection *source);

			//! Send header for data with a given size. After all data are send, the calling code must call source->asyncFinished to re-enable connection for commands.
			void sendAsyncDataHeader (size_t contentLength, XmlRpcServerConnection *source, const char *dataType = "binary/data");

		protected:
			XmlRpcServer* _server;

			XmlRpcServerConnection *connection;

			void addExtraHeader (const char *name, const char *value) { connection->addExtraHeader (name, value); }
			/**
			 * Specify max age in seconds. For this time cached response will be valid. This method
			 * is provide for convinient setting of cache timeout.
			 *
			 * @param maxage cache control max-age value (in seconds)
			 */
			void cacheMaxAge (long maxage)
			{
				std::ostringstream _os;
				_os << "max-age=" << maxage;
				connection->addExtraHeader ("Cache-Control", _os.str ());
			}
		private:
			std::string _prefix;
			const char *_description;

			std::string _request_base;

			std::string _username;
			std::string _password;
	};
}								 // namespace XmlRpc
#endif							 // _XMLRPCSERVERGETREQUEST_H_
