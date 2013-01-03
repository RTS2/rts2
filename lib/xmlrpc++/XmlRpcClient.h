
#ifndef _XMLRPCCLIENT_H_
#define _XMLRPCCLIENT_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)	 // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <string>
#endif

#include "XmlRpcDispatch.h"
#include "XmlRpcSource.h"

namespace XmlRpc
{

	// Arguments and results are represented by XmlRpcValues
	class XmlRpcValue;

	typedef enum {NOEXEC, XML_RPC, HTTP_GET, HTTP_POST} ExecutingType;

	//! A class to send XML RPC requests to a server and return the results.
	class XmlRpcClient : public XmlRpcSource
	{
		public:
			// Static data
			static const char REQUEST_BEGIN[];
			static const char REQUEST_END_METHODNAME[];
			static const char PARAMS_TAG[];
			static const char PARAMS_ETAG[];
			static const char PARAM_TAG[];
			static const char PARAM_ETAG[];
			static const char REQUEST_END[];
			// Result tags
			static const char METHODRESPONSE_TAG[];
			static const char FAULT_TAG[];

			//! Construct a client to connect to the server at the specified host:port address
			//!  @param path           The complete URI of requested resource
			//!  @param uri            String which will hold URI part of the resource
			XmlRpcClient(const char *path, const char **uri);

			//! Construct a client to connect to the server at the specified host:port address
			//!  @param host           The name of the remote machine hosting the server
			//!  @param port           The port on the remote machine where the server is listening
			//!  @param authorizationa Authorization string (ussually <username>:<password>)
			//!  @param uri            An optional string to be sent as the URI in the HTTP GET header
			XmlRpcClient(const char *host, int port, const char *authorization=NULL, const char *uri=NULL);

			//! Destructor
			virtual ~XmlRpcClient();

			//! Set client authorization
			void setAuthorization (const char *authorization)
			{
				if (authorization != NULL)
					_authorization = authorization;
				else
					_authorization = std::string("");
			}

			//! Execute the named procedure on the remote server.
			//!  @param method The name of the remote procedure to execute
			//!  @param params An array of the arguments for the method
			//!  @param result The result value to be returned to the client
			//!  @return true if the request was sent and a result received
			//!   (although the result might be a fault).
			//!
			//! Currently this is a synchronous (blocking) implementation (execute
			//! does not return until it receives a response or an error). Use isFault()
			//! to determine whether the result is a fault response.
			bool execute(const char* method, XmlRpcValue const& params, XmlRpcValue& result);

			//! Send GET request to remote server. You need to call
			//!   handleEvent from select call to retrieve data.
			//!  @param path  Server part of the URL.
			//!  @param reply Allocated reply buffer.
			//!  @param reply_size Size of reply in bytes.
			//!  @return true if page exists and some data were retrieved
			//!
			//! This is synchronous version of the request.
			bool executeGet(const char* path, char* &reply, int &reply_length);

			bool executeGetRequest(const char* path, const char *body, char* &reply, int &reply_length);

			bool executePost(const char* path, char* &reply, int &reply_length);

			bool executePostRequest(const char* path, const char *body, char* &reply, int &reply_length);

			//! Returns true if the result of the last execute() was a fault response.
			bool isFault() const { return _isFault; }

			// XmlRpcSource interface implementation
			//! Close the connection
			virtual void close();

			//! Handle server responses. Called by the event dispatcher during execute.
			//!  @param eventType The type of event that occurred.
			//!  @see XmlRpcDispatch::EventType
			virtual unsigned handleEvent(unsigned eventType);

			virtual void goAsync () {}

		protected:
			// Execution processing helpers
			virtual bool doConnect();
			virtual bool setupConnection();

			virtual bool generateRequest(const char* method, XmlRpcValue const& params);
			virtual bool generateGetRequest(const char* path, const char* body);
			virtual bool generatePostRequest(const char* path, const char* body);
			virtual std::string generateHeader(std::string const& body);
			virtual std::string generateGetPostHeader(std::string const& path, size_t contentLength, bool post);
			virtual bool writeRequest();
			virtual bool readHeader();
			virtual bool readResponse();
			virtual bool parseResponse(XmlRpcValue& result);

			// Possible IO states for the connection
			enum ClientConnectionState { NO_CONNECTION, CONNECTING, WRITE_REQUEST, READ_HEADER, READ_RESPONSE, IDLE };
			ClientConnectionState _connectionState;

			// Server location
			std::string _host;
			std::string _authorization;
			std::string _uri;
			int _port;

			std::string _proxy_host;
			int _proxy_port;

			char* _header;
			int _header_length;

			// The xml-encoded request, http header of response, and response xml
			std::string _request;

			char *_response_buf;
			int _response_length;

			// Number of times the client has attempted to send the request
			int _sendAttempts;

			// Number of bytes of the request that have been written to the socket so far
			size_t _bytesWritten;

			// Non-zero (no IDLE) if some request (either XML-RPC or HTTP GET) is executed. If you want to multithread,
			// each thread should have its own client.
			ExecutingType _executing;

			// True if the server closed the connection
			bool _eof;

			// True if a fault response was returned by the server
			bool _isFault;

			// Number of bytes expected in the response body (parsed from response header)
			int _contentLength;
			// Number of bytes in next chunk
			int _chunkLength;
			int _chunkReceivedLength;

			char *_chunkStart;
			char *_chunkEnd;

			// Event dispatcher
			XmlRpcDispatch _disp;
		private:
			void setupProxy ();
			void setupHost (const char *host, int port, const char *authorization, const char *uri);

			char *fullPath;

	};							 // class XmlRpcClient

}								 // namespace XmlRpc
#endif							 // _XMLRPCCLIENT_H_
