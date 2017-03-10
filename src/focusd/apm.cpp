#include "focusd.h"

#include "connection/apm.h"

#define	FOCUSER_WAIT	0
#define	FOCUSER_UP	1
#define FOCUSER_DOWN	2
#define FOCUSER_STOP	3
#define	FOCUSER_CALIB	4
#define	FOCUSER_LIMIT	5

namespace rts2focusd
{

	class APMFocuser: public Focusd
	{
		public:
			APMFocuser (int argc, char **argv);                
			virtual ~ APMFocuser (void);

		protected:
			virtual int isFocusing ();
			virtual bool isAtStartPosition ();

			virtual int processOption (int in_opt);
			virtual int initHardware ();
			virtual int commandAuthorized (rts2core::Rts2Connection *conn);
			virtual int info ();
			virtual int setTo (double num);
			virtual double tcOffset () { return 0.;};

		private:
			long steps;
			int focuser_state;
			HostString *host;
			rts2core::ConnAPM *connFocuser;
			rts2core::ValueString *focuser;
			int sendUDPMessage (const char * _message);
			void calibrate ();
			void stop ();
	};

};

using namespace rts2focusd;

APMFocuser::APMFocuser (int argc, char **argv):Focusd (argc, argv)
{
	host = NULL;
	focuser_state = -1;

	addOption ('e', NULL, 1, "APM focuser IP and port (separated by :)");

	createValue (focuser, "foc_state", "Focuser state", true);
}

APMFocuser::~APMFocuser (void)
{

}

int APMFocuser::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'e':
			host = new HostString (optarg, "1000");
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;	
}

int APMFocuser::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify dome hostname (with -e option)." << sendLog;
		return -1;
	}

	connFocuser = new rts2core::ConnAPM (host->getPort (), this, host->getHostname());

	int ret = connFocuser->init();

	if (ret)
		return ret;

	sendUDPMessage ("FO99999");

	return 0;
}

int APMFocuser::commandAuthorized (rts2core::Rts2Connection * conn)
{
	if (conn->isCommand ("calib"))
	{
		calibrate ();
	}

	if (conn->isCommand ("stop"))
	{
		stop ();
	}

	return 0;
}

void APMFocuser::calibrate ()
{
	sendUDPMessage ("FO99998");

	info ();
}

void APMFocuser::stop ()
{
	sendUDPMessage ("FOSTOP");

	info ();
}

int APMFocuser::info ()
{
	sendUDPMessage ("FO99999");

	logStream (MESSAGE_DEBUG) << "state: " << focuser_state << sendLog;	

	switch (focuser_state)
	{
		case FOCUSER_WAIT:
			setIdleInfoInterval (0);
			focuser->setValueString ("waiting");
			break; 
		case FOCUSER_UP:
			setIdleInfoInterval (1);
			focuser->setValueString ("moving up");
			break;
		case FOCUSER_DOWN:
			setIdleInfoInterval (1);
			focuser->setValueString ("moving down");
			break;
		case FOCUSER_STOP:
			setIdleInfoInterval (0);
			focuser->setValueString ("stopped");
			break;
		case FOCUSER_CALIB:
			setIdleInfoInterval (1);
			focuser->setValueString ("calibration");
			break;
		case FOCUSER_LIMIT:
			setIdleInfoInterval (0);
			focuser->setValueString ("limit reached");
			break;
		default:
			focuser->setValueString ("unknown");
	}

	sendValueAll (focuser);

	return Focusd::info ();
}

int APMFocuser::setTo (double num)
{
	int value = (int)num;
	if (value >= 0 && value < 28000)
	{	
		char * pos = (char *)malloc (5*sizeof(char));
		
		sprintf (pos, "FO%05d", (int)num);
		
		sendUDPMessage (pos);

		info ();
	}
	else
	{
		logStream (MESSAGE_ERROR) << "You must specify a value between [0..28000]" << sendLog;
	}

	return 0;
}

int APMFocuser::isFocusing ()
{
	sendUDPMessage ("FO99999");

	if ((focuser_state == FOCUSER_UP) || (focuser_state == FOCUSER_DOWN))
		return 10;
	else
		return -2;
}

bool APMFocuser::isAtStartPosition ()
{
	return 0;
/*
	sendUDPMessage ("FO99999");

	if (position == 0)
		return true;
	else 
		return false;
*/
}

int APMFocuser::sendUDPMessage (const char * _message)
{
	int old_focuser_state = focuser_state;
	bool wait = false;
	char *response = (char *)malloc(20*sizeof(char));

	logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

	if (!strcmp (_message, "FO99998") || !strcmp (_message, "FOSTOP"))
		wait = true;

	// int n = connFocuser->send (_message, response, 20, strcmp (_message, "FO99999"));

	int n = connFocuser->sendReceive (_message, response, 20, wait);

	if (n > 0)
	{
		std::string res (response);

		logStream (MESSAGE_DEBUG) << "response: " << res.substr(0,n) << sendLog;

		if (response[0] == 'F' && response[1] == 'O')
		{
			if (strlen (response) > 8)
			{
				steps = atol (res.substr(2, 6).c_str());
				logStream (MESSAGE_DEBUG) << "foc. position: " << steps << sendLog; 
				position->setValueDouble ((int) steps);

				if (response[7] == '-') 
					focuser_state = atoi (res.substr(8,1).c_str());
			}
			else
			{
				focuser_state = old_focuser_state;
			}
		}
	}

	return n;
}

int main (int argc, char **argv)
{
	APMFocuser device = APMFocuser (argc, argv);
	return device.run ();
}
