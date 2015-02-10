#include "focusd.h"

#include "connection/apm.h"

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
                virtual int info ();
                virtual int setTo (double num);
		virtual double tcOffset () { return 0.;};

	private:
		long steps;
		HostString *host;
                rts2core::ConnAPM *connFocuser;
		int sendUDPMessage (const char * _message);
};

};

using namespace rts2focusd;

APMFocuser::APMFocuser (int argc, char **argv):Focusd (argc, argv)
{
	host = NULL;

	addOption ('e', NULL, 1, "APM focuser IP and port (separated by :)");
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

	connFocuser = new rts2core::ConnAPM (this, host->getHostname(), host->getPort());
        
	int ret = connFocuser->init();
        
	if (ret)
                return ret;

        sendUDPMessage ("FO99999");

        return 0;
}

int APMFocuser::info ()
{
	sendUDPMessage ("FO99999");

        return Focusd::info ();
}

int APMFocuser::setTo (double num)
{
	char * pos = (char *)malloc (5*sizeof(char));

	sprintf (pos, "FO%.0f", num);

	sendUDPMessage (pos);

	return 0;
}

int APMFocuser::isFocusing ()
{
	return 0;
}

bool APMFocuser::isAtStartPosition ()
{
	return 0;
}

int APMFocuser::sendUDPMessage (const char * _message)
{

	char *response = (char *)malloc(20*sizeof(char));

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

        int n = connFocuser->send (_message, response, 20);

        logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

        if (n > 0)
        {
                if (response[0] == 'F' && response[1] == '0')
		{
			std::string res (response);
			steps = atol (res.substr(2, 6).c_str());
			position->setValueInteger ((int) steps);
		}
	}
	
	return 0;
}

int main (int argc, char **argv)
{
        APMFocuser device = APMFocuser (argc, argv);
        return device.run ();
}
