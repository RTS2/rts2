#include "../utils/rts2cliapp.h"

#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

class WeatherTimeout:public Rts2CliApp
{
	private:
		int timeout;			 // in seconds
		int udpPort;
		int waitTime;

	protected:
		virtual int processOption (int in_opt);
		virtual int doProcessing ();

	public:
		WeatherTimeout (int in_argc, char **in_argv);
};

WeatherTimeout::WeatherTimeout (int in_argc, char **in_argv):
Rts2CliApp (in_argc, in_argv)
{
	timeout = 3600;
	udpPort = 5002;
	waitTime = 100;

	addOption ('t', "timeout", 1, "timeout (in seconds) to send");
	addOption ('w', "wait_time", 1,
		"time for which we will wait for answer, defaults to 100 sec");
	addOption ('p', "udp_port", 1, "UDP port to which message will be send");
}


int
WeatherTimeout::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 't':
			timeout = atoi (optarg);
			break;
		case 'w':
			waitTime = atoi (optarg);
			if (waitTime < 1)
			{
				logStream (MESSAGE_WARNING)
					<< "Invalid wait time specified (" << waitTime <<
					"). Wait time is set to 1 second." << sendLog;
				waitTime = 1;
			}
			break;
		case 'p':
			udpPort = atoi (optarg);
			break;
		default:
			return Rts2CliApp::processOption (in_opt);
	}
	return 0;
}


void
sigAlarm (int sig)
{
	logStream (MESSAGE_ERROR) << "WeatherTimeout::run timeout" << sendLog;
	exit (1);
}


int
WeatherTimeout::doProcessing ()
{
	int sock;
	struct sockaddr_in bind_addr;
	struct sockaddr_in serv_addr;
	int ret;
	struct hostent *server_info;

	char buf[4];

	socklen_t size = sizeof (serv_addr);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror ("socket");
		return 1;
	}
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl (INADDR_ANY);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons (udpPort);

	server_info = gethostbyname ("localhost");
	if (!server_info)
	{
		perror ("gethostbyname localhost");
		return 2;
	}

	serv_addr.sin_addr = *(struct in_addr *) server_info->h_addr;

	ret = bind (sock, (struct sockaddr *) &bind_addr, sizeof (bind_addr));
	if (ret < 0)
	{
		perror ("bind");
		return 1;
	}

	std::ostringstream _os;
	_os << "weatherTimeout=" << timeout;

	ret =
		sendto (sock, _os.str ().c_str (), _os.str ().length (), 0,
		(struct sockaddr *) &serv_addr, sizeof (serv_addr));
	if (ret < 0)
	{
		perror ("write");
	}
	signal (SIGALRM, sigAlarm);
	alarm (waitTime);
	ret = recvfrom (sock, buf, 3, 0, (struct sockaddr *) &serv_addr, &size);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Error in recvfrom " << strerror (errno) <<
			sendLog;
		return 1;
	}

	buf[3] = 0;

	logStream (MESSAGE_DEBUG) << "Read " << buf << sendLog;

	close (sock);
	return 0;
}


int
main (int argc, char **argv)
{
	WeatherTimeout app = WeatherTimeout (argc, argv);
	return app.run ();
}
