#ifndef _XMLRPCSERVERCONNECTION_H_
#define _XMLRPCSERVERCONNECTION_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)	 // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <string>
#endif

#include <list>
#include <utility>

#include "XmlRpcValue.h"
#include "XmlRpcSource.h"

namespace XmlRpc
{

	// The server waits for client connections and provides methods
	class XmlRpcServer;
	class XmlRpcServerMethod;

	//! A class to handle XML RPC requests from a particular client
	class XmlRpcServerConnection : public XmlRpcSource
	{
		public:
			// Static data
			static const char METHODNAME_TAG[];
			static const char PARAMS_TAG[];
			static const char PARAMS_ETAG[];
			static const char PARAM_TAG[];
			static const char PARAM_ETAG[];

			static const std::string SYSTEM_MULTICALL;
			static const std::string METHODNAME;
			static const std::string PARAMS;

			static const std::string FAULTCODE;
			static const std::string FAULTSTRING;

			//! Constructor
			XmlRpcServerConnection(int fd, XmlRpcServer* server, bool deleteOnClose = false);
			//! Destructor
			virtual ~XmlRpcServerConnection();

			// XmlRpcSource interface implementation
			//! Handle IO on the client connection socket.
			//!   @param eventType Type of IO event that occurred. @see XmlRpcDispatch::EventType.
			virtual unsigned handleEvent(unsigned eventType);

			/**
			 * Add extra header among HTTP headers sent to client.
			 * @param name  Name of extra header
			 * @param value Extra header value
			 */
			void addExtraHeader (const char *name, const char *value) { _extra_headers.push_back (std::pair <const char *, const char *> (name, value)); }

		protected:

			bool readHeader();
			bool readRequest();
			bool handleGet();
			bool handlePost();
			bool writeResponse();

			// Parses the request, runs the method, generates the response xml.
			virtual void executeRequest();

			// Execute response to GET
			virtual void executeGet();

			// Parse the methodName and parameters from the request.
			std::string parseRequest(XmlRpcValue& params);

			// Execute a named method with the specified params.
			bool executeMethod(const std::string& methodName, XmlRpcValue& params, XmlRpcValue& result);

			// Execute multiple calls and return the results in an array.
			bool executeMulticall(const std::string& methodName, XmlRpcValue& params, XmlRpcValue& result);

			// Construct a response from the result XML.
			void generateResponse(std::string const& resultXml);
			void generateFaultResponse(std::string const& msg, int errorCode = -1);
			std::string generateHeader(std::string const& body);

			// The XmlRpc server that accepted this connection
			XmlRpcServer* _server;

			// Possible IO states for the connection
			enum ServerConnectionState { READ_HEADER, READ_REQUEST, GET_REQUEST, POST_REQUEST, WRITE_RESPONSE };
			ServerConnectionState _connectionState;

			// Request headers
			std::string _header;

			char* _header_buf;
			int _header_length;

			// User authorization
			std::string _authorization;

			// Name of data requested with GET
			std::string _get;

			// POST request
			std::string _post;

			// Number of bytes expected in the request body (parsed from header)
			int _contentLength;

			// Request body
			std::string _request;
			char* _request_buf;
			int _request_length;

			// Response
			std::string _response;

			// Number of bytes of the response written so far
			int _bytesWritten;

			// Response to GET request - header
			std::string _get_response_header;
			std::list <std::pair <const char*, const char*> > _extra_headers;

			// Response for GET request - data
			char *_get_response;
			size_t _get_response_length;

			// Number of bytes written for GET header and response so far
			size_t _getHeaderWritten;
			size_t _getWritten;

			// Whether to keep the current client connection open for further requests
			bool _keepAlive;
	};
}								 // namespace XmlRpc
#endif							 // _XMLRPCSERVERCONNECTION_H_
