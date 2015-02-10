
#include "XmlRpcSocketSSL.h"
#include "XmlRpcUtil.h"

#include <string.h>
#include <stdlib.h>

#include <iostream>

// OpenSSL
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#ifndef MAKEDEPEND

#if defined(_WINDOWS)
# include <stdio.h>
# include <winsock2.h>
//# pragma lib(WS2_32.lib)

# define EINPROGRESS  WSAEINPROGRESS
# define EWOULDBLOCK  WSAEWOULDBLOCK
# define ETIMEDOUT      WSAETIMEDOUT
#else
extern "C"
{
	# include <unistd.h>
	# include <stdio.h>
	# include <sys/types.h>
	# include <netdb.h>
	# include <errno.h>
	# include <fcntl.h>
}
#endif							 // _WINDOWS
#endif							 // MAKEDEPEND

using namespace XmlRpc;

SSL_CTX *XmlRpcSocket::ctx = NULL;
std::map<int, SSL*> XmlRpcSocket::ssl_map;

#if defined(_WINDOWS)

static void initWinSock()
{
	static bool wsInit = false;
	if (! wsInit)
	{
		WORD wVersionRequested = MAKEWORD( 2, 0 );
		WSADATA wsaData;
		WSAStartup(wVersionRequested, &wsaData);
		wsInit = true;
	}
}


#else

#define initWinSock()
#endif							 // _WINDOWS

// These errors are not considered fatal for an IO operation; the operation will be re-tried.
static inline bool
nonFatalError()
{
	int err = XmlRpcSocket::getError();
	return (err == EINPROGRESS || err == EAGAIN || err == EWOULDBLOCK || err == EINTR);
}

int
XmlRpcSocket::initSSL(const char *certFile, const char *keyFile)
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();

	ctx = SSL_CTX_new(SSLv23_server_method());

	// load cacert key pair
	SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM);
	if (!SSL_CTX_check_private_key(ctx))
		return -1;

	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	return 0;
}

int
XmlRpcSocket::socket()
{
	initWinSock();
	return (int) ::socket(AF_INET, SOCK_STREAM, 0);
}


void
XmlRpcSocket::close(int fd)
{
	XmlRpcUtil::log(4, "XmlRpcSocket::close: fd %d.", fd);
	#if defined(_WINDOWS)
	closesocket(fd);
	#else
	::close(fd);
	#endif						 // _WINDOWS
}

bool
XmlRpcSocket::setNonBlocking(int fd)
{
	#if defined(_WINDOWS)
	unsigned long flag = 1;
	return (ioctlsocket((SOCKET)fd, FIONBIO, &flag) == 0);
	#else
	return (fcntl(fd, F_SETFL, O_NONBLOCK) == 0);
	#endif						 // _WINDOWS
}

bool
XmlRpcSocket::unsetNonBlocking(int fd)
{
	#if defined(_WINDOWS)
	unsigned long flag = 1;
	return (ioctlsocket((SOCKET)fd, 0, &flag) == 0);
	#else
	return (fcntl(fd, F_SETFL, 0) == 0);
	#endif						 // _WINDOWS
}

bool
XmlRpcSocket::setReuseAddr(int fd)
{
	// Allow this port to be re-bound immediately so server re-starts are not delayed
	int sflag = 1;
	return (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&sflag, sizeof(sflag)) == 0);
}


// Bind to a specified port
bool
XmlRpcSocket::bind(int fd, int port)
{
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons((u_short) port);
	return (::bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == 0);
}


// Set socket in listen mode
bool
XmlRpcSocket::listen(int fd, int backlog)
{
	return (::listen(fd, backlog) == 0);
}


int
#ifdef _WINDOWS
XmlRpcSocket::accept(int fd, struct sockaddr_in &addr, int &addrlen)
#else
XmlRpcSocket::accept(int fd, struct sockaddr_in &addr, socklen_t &addrlen)
#endif
{
	addrlen = sizeof(addr);

	int _fd = ::accept(fd, (struct sockaddr*)&addr, &addrlen);
	SSL *ssl = SSL_new(ctx);

	SSL_set_fd(ssl, _fd);
	SSL_accept(ssl);
	ssl_map[_fd] = ssl;

	return _fd;
}


// Connect a socket to a server (from a client)
bool
XmlRpcSocket::connect(int fd, std::string& host, int port)
{
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;

	struct hostent *hp = gethostbyname(host.c_str());
	if (hp == 0) return false;

	saddr.sin_family = hp->h_addrtype;
	memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);
	saddr.sin_port = htons((u_short) port);

	// For asynch operation, this will return EWOULDBLOCK (windows) or
	// EINPROGRESS (linux) and we just need to wait for the socket to be writable...
	int result = ::connect(fd, (struct sockaddr *)&saddr, sizeof(saddr));
	return result == 0 || nonFatalError();
}


// Read available text from the specified socket. Returns false on error.
bool
XmlRpcSocket::nbRead(int fd, char* &s, int &l, bool *eof)
{
	const int READ_SIZE = 4096;	 // Number of bytes to attempt to read at a time
	char readBuf[READ_SIZE];

	bool wouldBlock = false;
	*eof = false;

	SSL *ssl = ssl_map[fd];

	while ( ! wouldBlock && ! *eof)
	{
		int n = SSL_read(ssl, readBuf, READ_SIZE-1);
		XmlRpcUtil::log(5, "XmlRpcSocket::nbRead: read/recv returned %d.", n);

		if (n > 0)
		{
			readBuf[n] = 0;
			s = (char*) realloc(s, l + n + 1);
			memcpy(s + l, readBuf, n + 1);
			l += n;
		}
		else if (n == 0)
		{
			*eof = true;
		}
		else if (nonFatalError())
		{
			wouldBlock = true;
		}
		else
		{
			return false;		 // Error
		}
	}
	return true;
}


// Write text to the specified socket. Returns false on error.
size_t
XmlRpcSocket::nbWrite(int fd, std::string s, size_t *bytesSoFar, bool sendfull)
{
	int nToWrite = int(s.length()) - *bytesSoFar;
	char *sp = const_cast<char*>(s.c_str()) + *bytesSoFar;

	SSL *ssl = ssl_map[fd];
	while ( nToWrite > 0 )
	{
		int n = SSL_write(ssl, sp, nToWrite);

		if (n > 0)
		{
			sp += n;
			*bytesSoFar += n;
			nToWrite -= n;
			XmlRpcUtil::log(5, "XmlRpcSocket::nbWrite: send/write returned %d.", n);
		}
		else if (nonFatalError())
		{
			if (!sendfull)
				return n;
		}
		else
		{
			XmlRpcUtil::log(5, "XmlRpcSocket::nbWrite: send error %s (%d)", strerror(errno), errno);
			return -1;		 // Error
		}
	}
	return 0;
}


// Write text to the specified socket. Returns false on error.
size_t
XmlRpcSocket::nbWriteBuf(int fd, const char *buf, size_t buf_len, size_t *bytesSoFar, bool sendfull, bool retry)
{
	size_t nToWrite = buf_len - *bytesSoFar;

	SSL *ssl = ssl_map[fd];
	while ( nToWrite > 0 )
	{
		int n = SSL_write(ssl, buf + *bytesSoFar, nToWrite);
		XmlRpcUtil::log(5, "XmlRpcSocket::nbWrite: send/write returned %d.", n);

		if (n > 0)
		{
			*bytesSoFar += n;
			nToWrite -= n;
			if (retry == false)
				return 0;
		}
		else if (nonFatalError())
		{
			if (retry == false)
				return 0;
			if (!sendfull)
				return n;
		}
		else
		{
			return -1;		 // Error
		}
	}
	return 0;
}



// Returns last errno
int
XmlRpcSocket::getError()
{
	#if defined(_WINDOWS)
	return WSAGetLastError();
	#else
	return errno;
	#endif
}


// Returns message corresponding to last errno
std::string
XmlRpcSocket::getErrorMsg()
{
	return getErrorMsg(getError());
}


// Returns message corresponding to errno... well, it should anyway
std::string
XmlRpcSocket::getErrorMsg(int error)
{
	char err[120];
        #if defined(_WINDOWS)
	snprintf(err,sizeof(err),"error %d", error);
	#else
	snprintf(err,sizeof(err),"error %d(%s)", error, strerror(error));
	#endif
	return std::string(err);
}
