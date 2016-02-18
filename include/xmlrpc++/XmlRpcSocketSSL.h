#ifndef _XMLRPCSOCKETSSL_H_
#define _XMLRPCSOCKETSSL_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)	 // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <string>
#endif

#include <map>

#ifdef _WINDOWS
#include <winsock2.h>
#else
extern "C"
{
	#include <sys/types.h>
	#include <netdb.h>
	#include <netinet/in.h>
}
#endif

// OpenSSL
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#include "XmlRpcSocket.h"

namespace XmlRpc
{

	//! A platform-independent socket API.
	class XmlRpcSocketSSL: public XmlRpcSocket
	{
		public:

			//! Creates a stream (TCP) socket. Returns -1 on failure.
			static int socket();

			//! Closes a socket.
			static void close(int socket);

			//! Sets a stream (TCP) socket to perform non-blocking IO. Returns false on failure.
			static bool setNonBlocking(int socket);

			//! unSets a stream (TCP) socket to perform blocking IO. Returns false on failure.
			static bool unsetNonBlocking(int socket);

			//! Read text from the specified socket. Returns false on error.
			static bool nbRead(int socket, char* &s, int &l, bool *eof);

			//! Write text to the specified socket. Returns false on error.
			static size_t nbWrite(int socket, std::string s, size_t *bytesSoFar, bool sendfull = true);

			//! Write buffer to the specified socket. Returns false on error.
			static size_t nbWriteBuf(int socket, const char *buf, size_t buf_len, size_t *bytesSoFar, bool sendfull = true, bool retry = true);

			// The next four methods are appropriate for servers.

			//! Allow the port the specified socket is bound to to be re-bound immediately 
			//! server re-starts are not delayed. Returns false on failure.
			static bool setReuseAddr(int socket);

			//! Bind to a specified port
			static bool bind(int socket, int port);

			//! Set socket in listen mode
			static bool listen(int socket, int backlog);

			//! Accept a client connection request
#ifdef _WINDOWS
			static int accept(int socket, struct sockaddr_in &addr, int &addrlen);
#else
			static int accept(int socket, struct sockaddr_in &addr, socklen_t &addrlen);
#endif

			//! Connect a socket to a server (from a client)
			static bool connect(int socket, std::string& host, int port);

			//! Returns last errno
			static int getError();

			//! Returns message corresponding to last error
			static std::string getErrorMsg();

			//! Returns message corresponding to error
			static std::string getErrorMsg(int error);

			//! Initialize OpenSSL Context
			static int initSSL(const char *certFile, const char *keyFile);

			//! OpenSSL Context
			static SSL_CTX *ctx;

			//! SSL object map
			static std::map<int, SSL*> ssl_map;
	};

}								 // namespace XmlRpc
#endif
