
#ifndef _XMLRPCEXCEPTION_H_
#define _XMLRPCEXCEPTION_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)	 // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <string>
#endif

#include <exception>
#include <ostream>

#define HTTP_OK              200
#define HTTP_BAD_REQUEST     400
#define HTTP_UNAUTHORIZED    401

namespace XmlRpc
{

	//! A class representing an error.
	//! If server methods throw this exception, a fault response is returned
	//! to the client.
	class XmlRpcException: public std::exception
	{
		public:
			//! Constructor
			//!   @param what  A descriptive error message
			//!   @param code     An integer error code
			explicit XmlRpcException(const std::string& message, int code=-1) :
			std::exception (), _message(message), _code(code) {}

			virtual ~XmlRpcException () throw () {}

			virtual const char* what () const throw () { return _message.c_str (); }

			//! Return the error message.
			const std::string& getMessage() const { return _message; }

			//! Return the error code.
			int getCode() const { return _code; }

		private:
			std::string _message;
			int _code;
			
			friend std::ostream & operator << (std::ostream &_os, XmlRpcException &ex)
			{
				_os << ex.getMessage () << " " << ex.getCode ();
				return _os;
			}
	};

	//! A class thrown to indicate problem in which should be thrown as JSON error message.
	class JSONException:public std::exception
	{
		public:
			explicit JSONException(const std::string& message, int code = HTTP_BAD_REQUEST) : std::exception (), _message(message), _code (code) {}

			virtual ~JSONException () throw () {}

			//! Return the error message.
			const std::string& getMessage() const { return _message; }
			int getCode () const { return _code; }
		private:
			std::string _message;
			int _code;
	};
}
#endif							 // _XMLRPCEXCEPTION_H_
