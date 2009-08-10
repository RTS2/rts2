
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
#endif

#define HTTP_OK         200
#define HTTP_FAILED     400
#define HTTP_AUTHORIZE  401

namespace XmlRpc
{
	// The XmlRpcServer processes client requests to call GET methods.
	class XmlRpcServer;

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
			virtual void execute(const char* path, int &http_code, const char* &response_type, char* &respose, int &response_length) = 0;

			//! Returns 401 page
			virtual void authorizePage(int &http_code, const char* &response_type, char* &response, int &response_length);

		protected:
			std::string _prefix;
			XmlRpcServer* _server;
		private:
			std::string _username;
			std::string _password;
	};
}								 // namespace XmlRpc
#endif							 // _XMLRPCSERVERGETREQUEST_H_
