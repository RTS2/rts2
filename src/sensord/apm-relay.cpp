#include "sensord.h"

#include "connection/apm.h"

#define	RELAYS_OFF	1
#define RELAY1_ON	2
#define	RELAY2_ON	3
#define RELAYS_ON	4

namespace rts2sensord
{

class APMRelay : public Sensor
{
	public:
		APMRelay (int argc, char **argv);
		virtual int initHardware ();
		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual int info ();
	protected:
		virtual int processOption (int in_opt);
	private:
		int relays_state;
		HostString *host;
		rts2core::ConnAPM *connRelay;
		rts2core::ValueString *relay1, *relay2;
		int relayOn (int n);
		int relayOff (int n);
		int sendUDPMessage (const char * _message);
};

}

using namespace rts2sensord;

int APMRelay::processOption (int opt)
{
	switch (opt)
	{
		case 'e':
			host = new HostString (optarg, "1000");
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int APMRelay::initHardware ()
{
	if (host == NULL)	        
	{
		logStream (MESSAGE_ERROR) << "You must specify dome hostname (with -e option)." << sendLog;
		return -1;	
	}

	connRelay = new rts2core::ConnAPM (this, host->getHostname(), host->getPort());
	int ret = connRelay->init();
	if (ret)
		return ret;

	sendUDPMessage ("A999");

	return 0;
}

int APMRelay::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("on"))
	{
		int n;
		if (conn->paramNextInteger (&n) || !conn->paramEnd ())
			return -2;
		return relayOn (n);
	}
	
	if (conn->isCommand ("off"))
	{
		int n;
		if (conn->paramNextInteger (&n) || !conn->paramEnd ())
			return -2;
		return relayOff (n);
	}

	return 0;
}

int APMRelay::info ()
{
	sendUDPMessage ("A999");

	switch (relays_state)
	{
		case RELAYS_OFF:
			relay1->setValueString("off");
			relay2->setValueString("off");
			break;
		case RELAY1_ON:
			relay1->setValueString("on");
			relay2->setValueString("off");
			break;
		case RELAY2_ON:
			relay1->setValueString("off");
			relay2->setValueString("on");
			break;
		case RELAYS_ON:
			relay1->setValueString("on");
			relay2->setValueString("on");
			break;
	}

	sendValueAll(relay1);
	sendValueAll(relay2);

	return Sensor::info();
}

int APMRelay::relayOn (int n)
{
	char *cmd = (char *)malloc(4*sizeof (char));

	sprintf (cmd, "A00%d", n);

	sendUDPMessage (cmd);
	sendUDPMessage ("A999");

	return 0;
}

int APMRelay::relayOff (int n)
{
	// current version of firmware doens't allow separate switch off
	sendUDPMessage ("A000");
	sendUDPMessage ("A999");
	return 0;
}

int APMRelay::sendUDPMessage (const char * _message)
{
	char *response = (char *)malloc(20*sizeof(char));

	logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

	int n = connRelay->send (_message, response, 20);

	logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

	if (n > 0)
	{
		if (response[0] == 'A' && response[1] == '9')
		{
			switch (response[3])
			{
				case '0':
					relays_state = RELAYS_OFF;
					break;
				case '1':
					relays_state = RELAY1_ON;
					break;
				case '2':
					relays_state = RELAY2_ON;
					break;
				case '3':
					relays_state = RELAYS_OFF;
					break;
			}
		}
	}

	return 0;
}

APMRelay::APMRelay (int argc, char **argv): Sensor (argc, argv)
{
	host = NULL;

        addOption ('e', NULL, 1, "ESA dome IP and port (separated by :)");
	createValue(relay1, "relay1", "Relay 1 state", true);
	createValue(relay2, "relay2", "Relay 2 state", true);
}

int main (int argc, char **argv)
{
	APMRelay device (argc, argv);
	return device.run ();
}
