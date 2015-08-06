#include "apm-mirror.h"
#include "connection/apm.h"

using namespace rts2sensord;

APMMirror::APMMirror (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter): Sensor (argc, argv, sn)
{
	addOption ('e', NULL, 1, "IP and port (separated by :)");
	addOption ('F', NULL, 1, "filter names, separated by : (double colon)");

	createValue (mirror, "mirror", "Mirror cover state", true);

	filter = in_filter;
}

int APMMirror::initHardware ()
{
	connMirror = filter->connFilter;

	sendUDPMessage ("C999");

	return 0;
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

void APMMirror::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			{
				switch (new_state & SERVERD_ONOFF_MASK)
				{
					case SERVERD_ON:
						open ();
						break;
					default:
						close();
						break;
				}
			}
			break;
		default:
			close();
			break;
	}
	Sensor::changeMasterState (old_state, new_state);
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
		case MIRROR_OPENING:
			mirror->setValueString("opening");
			break;
		case MIRROR_CLOSING:
			mirror->setValueString("closing");
			break;
	}

	sendValueAll(mirror);

	return Sensor::info ();
}

int APMMirror::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'e':
		case 'F':
			return 0;
		default:
			return Sensor::processOption (in_opt);
	}
}

int APMMirror::open ()
{
	sendUDPMessage ("C001");

	if (mirror_state == MIRROR_CLOSED)
		mirror_state = MIRROR_OPENING;

	info ();

	return 0;
}

int APMMirror::close ()
{
	sendUDPMessage ("C000");

	if (mirror_state == MIRROR_OPENED)
		mirror_state = MIRROR_CLOSING;

	info ();

	return 0;
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
					if (mirror_state != MIRROR_OPENING)
                                        	mirror_state = MIRROR_CLOSED;
                                        break;
                                case '1':
					if (mirror_state != MIRROR_CLOSING)
                                        	mirror_state = MIRROR_OPENED;
                                        break;
                        }
                }
        }

        return 0;
}
