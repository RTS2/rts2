#include "sensord.h"
#include "connection/apm.h"

#define MIRROR_OPENED   1
#define MIRROR_CLOSED   2

namespace rts2sensord
{

/**
 * APM mirror cover driver.
 *
 * @author Stanislav Vitek <standa@vitkovi.net>
 */
class APMMirror : public Sensor
{
        public:
                APMMirror (int argc, char **argv);
		virtual int processOption (int in_opt);
                virtual int initHardware ();
                virtual int commandAuthorized (rts2core::Connection *conn);
                virtual int info ();
        private:
                int mirror_state;
                HostString *host;
                rts2core::ConnAPM *connMirror;
                rts2core::ValueString *mirror;
                int open ();
                int close ();
                int sendUDPMessage (const char * _message);
};

}

using namespace rts2sensord;

APMMirror::APMMirror (int argc, char **argv):Sensor (argc, argv)
{
        host = NULL;

        addOption ('e', NULL, 1, "IP and port (separated by :)");
}

int APMMirror::processOption (int in_opt)
{
        switch (in_opt)
        {
                case 'e':
                        host = new HostString (optarg, "1000");
                        break;
                default:
                        return Sensor::processOption (in_opt);
        }
        return 0;
}

int APMMirror::initHardware ()
{
	if (host == NULL)
        {
                logStream (MESSAGE_ERROR) << "You must specify filter hostname (with -e option)." << sendLog;
                return -1;
        }

        connMirror = new rts2core::ConnAPM (this, host->getHostname (), host->getPort ());

        int ret = connMirror->init();
        if (ret)
                return ret;

        return info();
}

int APMMirror::commandAuthorized (rts2core::Connection * conn)
{
        if (conn->isCommand ("open"))
        {
                return open();
        }

        if (conn->isCommand ("close"))
        {
                return close();
        }

        return 0;
}

int APMMirror::info ()
{
        sendUDPMessage ("C999");

        switch (mirror_state)
        {
                case MIRROR_OPENED:
                        mirror->setValueString("opened");
                        break;
                case MIRROR_CLOSED:
                        mirror->setValueString("closed");
                        break;
        }

        sendValueAll(mirror);

        return Sensor::info ();
}

int APMMirror::open ()
{
        sendUDPMessage ("C001");

        return info ();
}

int APMMirror::close ()
{
        sendUDPMessage ("C000");

        return info ();
}

int APMMirror::sendUDPMessage (const char * _message)
{
        char *response = (char *)malloc(20*sizeof(char));

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

        int n = connMirror->send (_message, response, 20);

        logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

        if (n > 0)
        {
                if (response[0] == 'C' && response[1] == '9')
                {
                        switch (response[3])
                        {
                                case '0':
                                        mirror_state = MIRROR_OPENED;
                                        break;
                                case '1':
                                        mirror_state = MIRROR_CLOSED;
                                        break;
                        }
                }
        }

        return 0;
}

int main (int argc, char **argv)
{
	APMMirror device (argc, argv);
        return device.run ();
}
