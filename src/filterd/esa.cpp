#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include "filterd.h"

using namespace rts2filterd;

class EsaFilter : public Filterd
{
	public:
		EsaFilter (int in_argc, char **in_argv);
		virtual int initHardware ();
	protected:
		virtual int processOption (int in_opt);
		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);
		virtual int homeFilter ();
	private:
		HostString *host;
		int sock;
		struct sockaddr_in servaddr, clientaddr;
		// rts2core::ConnAPM *apm;
		int sendUDPMessage (const char * in_message);
		int filterNum;
		int filterSleep;
};


EsaFilter::EsaFilter (int argc, char **argv):Filterd (argc, argv)
{
	host = NULL;
	filterNum = 0;
	filterSleep = 3;

	addOption ('e', NULL, 1, "ESA filter IP and port (separated by :)");
	addOption ('s', "filter_sleep", 1, "how long wait for filter change");
}

int EsaFilter::processOption (int in_opt)
{
        switch (in_opt)
        {
                case 'e':
                        host = new HostString (optarg, "1000");
                        break;
		case 's':
			filterSleep = atoi (optarg);
			break;
                default:
                        return Filterd::processOption (in_opt);
        }
        return 0;
}

int EsaFilter::getFilterNum ()
{
	sendUDPMessage ("F999");

	return filterNum;
}

int EsaFilter::setFilterNum (int new_filter)
{
	char *m = (char *)malloc(5*sizeof(char));
	sprintf (m, "F00%d", new_filter);

	sendUDPMessage (m);

	filterNum = new_filter;

	sleep (filterSleep);

	return Filterd::setFilterNum (new_filter);
}

int EsaFilter::homeFilter ()
{
	setFilterNum (0);
	
	return 0;
}

int EsaFilter::initHardware ()
{
	if (host == NULL)
        {
                logStream (MESSAGE_ERROR) << "You must specify filter hostname (with -e option)." << sendLog;
                return -1;
        }

	// apm = new rts2core::ConnAPM (this, host->getHostname (), host->getPort ());

	sock = socket (AF_INET, SOCK_DGRAM, 0);

        bzero (&servaddr, sizeof (servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr (host->getHostname());
        servaddr.sin_port = htons (host->getPort());

        bzero (&clientaddr, sizeof (clientaddr));
        clientaddr.sin_family = AF_INET;
        clientaddr.sin_port = htons (host->getPort());
        clientaddr.sin_addr.s_addr = htonl (INADDR_ANY);

        if (bind (sock , (struct sockaddr*)&clientaddr, sizeof(clientaddr) ) == -1)
        {
                logStream (MESSAGE_ERROR) << "Unable to bind socket to port" << sendLog;
                return 1;
        }	

	return 0;
}

int EsaFilter::sendUDPMessage (const char * in_message)
{
	unsigned int slen = sizeof (clientaddr);
        char * status_message = (char *)malloc (20*sizeof (char));

	logStream (MESSAGE_DEBUG) << "command to controller: " << in_message << sendLog;

	sendto (sock, in_message, strlen(in_message), 0, (struct sockaddr *)&servaddr,sizeof(servaddr));

	int n = recvfrom (sock, status_message, 20, 0, (struct sockaddr *) &clientaddr, &slen);

	logStream (MESSAGE_DEBUG) << "reponse from controller: " << status_message << sendLog;

	return 0;
}

int main (int argc, char ** argv)
{
        EsaFilter device (argc, argv);

        return device.run ();
}
