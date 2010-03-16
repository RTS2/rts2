
#ifndef _XMLRPCSERVERGETREQUEST_H_
#define _XMLRPCSERVERGETREQUEST_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)	 // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <string>
# include <vector>
#endif

#include "XmlRpcServerConnection.h"

#define HTTP_OK              200
#define HTTP_BAD_REQUEST     400
#define HTTP_UNAUTHORIZED    401

namespace XmlRpc
{
	// The XmlRpcServer processes client requests to call GET methods.
	class XmlRpcServer;

	/**
	 * Parameter passed on GET request.
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

	class HttpParams:public std::vector <HttpParam>
	{
		public:
			HttpParams () {};
			void addParam (std::string _name, std::string _val) { push_back (HttpParam (_name, _val)); }

			std::string getValue (const char *_name, const char *dev_val);
			const char *getString (const char *_name, const char *def_val);
			int getInteger (const char *_name, int def_val);
			double getDouble (const char *_name, double def_val);

			/** 
			 * Return date as double ctime.
			 * 
			 * @param _name  name of variable which will be returned.
			 * @param def_val default value. You can use time(NULL) to use current time.
			 * 
			 * @return 
			 */
			double getDate (const char *_name, double def_val);

			void parseParam (const std::string& ps);
			void parse (const std::string &ps);
	};

	//! Abstract class representing a single GET request
	class XmlRpcServerGetRequest
	{
		public:
			//! Constructor
			XmlRpcServerGetRequest(std::string const& prefix, XmlRpcServer* server = 0);
			//! Destructor
			virtual ~XmlRpcServerGetRequest();

			//! Returns the name of the method
			std::string& getPrefix() { return _prefix; }

			//! Set authentification string..
			void setAuthorization(std::string authorization);

			//! Return username
			std::string getUsername() { return _username; }

			//! Return user password
			std::string getPassword() { return _password; }

			//! Execute the method. Subclasses must provide a definition for this method.
			virtual void execute(std::string path, HttpParams *params, int &http_code, const char* &response_type, char* &respose, size_t &response_length) = 0;
			//! Returns 401 page
			virtual void authorizePage(int &http_code, const char* &response_type, char* &response, size_t &response_length);

			void setConnection (XmlRpcServerConnection *_connection) { connection = _connection; }

		protected:
			std::string _prefix;
			XmlRpcServer* _server;

			void addExtraHeader (const char *name, const char *value) { connection->addExtraHeader (name, value); }
		private:
			std::string _username;
			std::string _password;

			XmlRpcServerConnection *connection;
	};
}								 // namespace XmlRpc
#endif							 // _XMLRPCSERVERGETREQUEST_H_
