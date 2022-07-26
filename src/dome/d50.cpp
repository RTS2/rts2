/*
 * Driver for D50 dome in Ondrejov, using unit, made by Martin ("Ford") Nekola.
 * Copyright (C) 2022 Jan Strobl
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "domeford.h"

#define TIME_TO_OPEN_DOME              30
// this is to surely cross the EMI-caused false signal of endswitch being connected
#define MINIMAL_TIME_TO_MOVE              2

// domeMotorState selection values
#define DOME_MOTOR_STATE_OFF		0
#define DOME_MOTOR_STATE_OPENING	1
#define DOME_MOTOR_STATE_CLOSING	2

#define OPT_MOUNT_DEVICE		OPT_LOCAL + 150
#define OPT_MOUNT_INDOME_VARIABLE	OPT_LOCAL + 151

typedef enum
{
	OTEVIRANI,
	ZAVIRANI,
	VYSTUP_3,
	VYSTUP_4,
	VYSTUP_5,
	VYSTUP_6,
	VYSTUP_7,
	UNDEFINED_B_128,
	UNDEFINED_A_1,
	KONCAK_OTEVRENI,
	KONCAK_ZAVRENI,
	VSTUP_3,
	VSTUP_4,
	UNDEFINED_A_32,
	UNDEFINED_A_64,
	UNDEFINED_A_128
} vystupy;


using namespace rts2dome;

namespace rts2dome
{

/**
 * Class for D50 dome control.
 *
 * @author Jan Strobl
 * @author Petr Kubanek <petr@kubanek.net>
 */
class D50:public Ford
{
	private:
		time_t timeToMoveMax, timeToMoveMin;

		rts2core::ValueInteger *sw_state;
		rts2core::ValueSelection *domeMotorState, *domeMotorStateOpening, *domeMotorStateClosing, *domeSwitchOpened, *domeSwitchClosed;

		const char *mountDevice;
		const char *mountInsideDomeVariable;

		rts2core::ValueBool *ignoreMountState;

		/**
		* Read data from (external) mount device and find out if the dome can be safely closed.
		*
		* @return zero (0) if the dome is parked or inside "closed-dome" safe area, -1 if an error occured, 1 when parking in progress, 2 otherwise
		*/
		int getMountState ();

	protected:
		virtual int init ();
		virtual int info ();
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int willConnect (rts2core::NetworkAddress * in_addr);
		virtual int processOption (int in_opt);

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();
		virtual int stop ();

	public:
		D50 (int argc, char **argv);
		virtual ~D50 ();
};

}

D50::D50 (int argc, char **argv):Ford (argc, argv)
{
	timeToMoveMax = 0;
	timeToMoveMin = 0;

	createValue (sw_state, "sw_state", "state of dome switches", false, RTS2_DT_HEX);
	createValue (domeMotorState, "motor_state", "dome-motor state", false);
	domeMotorState->addSelVal ("off");
	domeMotorState->addSelVal ("opening (moving)");
	domeMotorState->addSelVal ("closing (moving)");
	createValue (domeMotorStateOpening, "motor_state_opening", "motor opening output state", false, RTS2_DT_ONOFF);
	domeMotorStateOpening->addSelVal ("---");
	domeMotorStateOpening->addSelVal ("OPENING");
	createValue (domeMotorStateClosing, "motor_state_closing", "motor closing output state", false, RTS2_DT_ONOFF);
	domeMotorStateClosing->addSelVal ("---");
	domeMotorStateClosing->addSelVal ("CLOSING");
	createValue (domeSwitchOpened, "roof_opened", "switch on south roof, opened state", false, RTS2_DT_ONOFF);
	domeSwitchOpened->addSelVal ("---");
	domeSwitchOpened->addSelVal ("OPENED");
	createValue (domeSwitchClosed, "roof_closed", "switch on south roof, closed state", false, RTS2_DT_ONOFF);
	domeSwitchClosed->addSelVal ("---");
	domeSwitchClosed->addSelVal ("CLOSED");

	addOption (OPT_MOUNT_DEVICE, "mount-device", 1, "name of mount device [T0]");
	addOption (OPT_MOUNT_INDOME_VARIABLE, "mount-indome-variable", 1, "variable name of the mount device showing the mount is inside iof the dome [inside_dome_limit]");
	mountDevice = "T0";
	mountInsideDomeVariable = "inside_dome_limit";

	createValue (ignoreMountState, "ignore_mount_state", "ignore state of mount (teld) when closing the dome (DANGEROUS!)", false, RTS2_VALUE_WRITABLE);
	ignoreMountState->setValueBool (false);

	setIdleInfoInterval(5);
}

D50::~D50 ()
{
}

int D50::startOpen ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "startOpen: zjisti_stav_portu () error" << sendLog;
		return ret;	// error
	}

	if (getPortState (OTEVIRANI))
	{
		// opening already in progress
		return 0;
	}

	if (getPortState (ZAVIRANI))
	{
		// the dome is just closing, but this is a decent dome, we can simply stop, wait a bit and then start the opening as requested
		VYP (ZAVIRANI);
		sleep (2);
	}

	// TODO: Pridat check na reseni zaparkovani dalekohledu atd.

	ZAP (OTEVIRANI);
	logStream (MESSAGE_DEBUG) << "opening the dome" << sendLog;

	time (&timeToMoveMax);
	timeToMoveMin = timeToMoveMax + MINIMAL_TIME_TO_MOVE;
	timeToMoveMax += TIME_TO_OPEN_DOME;

	return 0;
}

long D50::isOpened ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (timeToMoveMax != 0 && timeToMoveMax < time (NULL))
	{
		logStream (MESSAGE_ERROR) << "A timeout reached during the dome opening!" << sendLog;
		return -1;
	}
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "isOpened: zjisti_stav_portu () error" << sendLog;
		return ret;	// error
	}
	if (getPortState (KONCAK_OTEVRENI) && timeToMoveMin < time (NULL))
		return -2;	// dome is open!
	return USEC_SEC;
}

int D50::endOpen ()
{
	int ret;
	VYP (OTEVIRANI);
	ret = zjisti_stav_portu ();	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "endOpen: zjisti_stav_portu () error" << sendLog;
		return ret;	// error
	}

	timeToMoveMax = 0;
	setTimeout (2 * USEC_SEC);
	if (!getPortState (KONCAK_OTEVRENI))
		return -1;	// dome is not open properly :-(

	return 0;
}

int D50::startClose ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "startClose: zjisti_stav_portu () error" << sendLog;
		return ret;	// error
	}

	if (!ignoreMountState->getValueBool ())
	{
		int mountState = getMountState ();
		logStream (MESSAGE_DEBUG) << "startClose: mountState: " << mountState << sendLog;
		switch (mountState)
		{
			case -1:
				logStream (MESSAGE_ERROR) << "startClose: Cannot close dome, mount is not responding (use 'ignore_mout_state' switch to bypass, but BE SURE the mount is really parked before!)." << sendLog;
				return -1;
				break;
			case 1:
				logStream (MESSAGE_DEBUG) << "startClose: Mount is not within limits at the moment, parking is already in progress" << sendLog;
				return 1;
				break;
			case 2:
				logStream (MESSAGE_DEBUG) << "startClose: Mount is not within limits at the moment, sending 'park' command." << sendLog;
				rts2core::Connection *connMount = NULL;
				connMount = getOpenConnection (mountDevice);
				if (connMount == NULL)
				{
					logStream (MESSAGE_ERROR) << "startClose: device connection error!" << sendLog;
					return -1;
				}
				connMount->queCommand (new rts2core::Command (this, COMMAND_TELD_PARK));
				return 1;
				break;
		}
	}

	if (getPortState (ZAVIRANI))
	{
		// closing already in progress
		return 0;
	}

	if (getPortState (OTEVIRANI))
	{
		// the dome is just opening, but this is a decent dome, we can simply stop, wait a bit and then start the closing as requested
		VYP (OTEVIRANI);
		sleep (2);
	}

	ZAP (ZAVIRANI);
	logStream (MESSAGE_DEBUG) << "closing the dome" << sendLog;

	time (&timeToMoveMax);
	timeToMoveMin = timeToMoveMax + MINIMAL_TIME_TO_MOVE;
	timeToMoveMax += TIME_TO_OPEN_DOME;

	return 0;
}

long D50::isClosed ()
{
	int ret;

	if (timeToMoveMax != 0 && timeToMoveMax < time (NULL))
	{
		logStream (MESSAGE_ERROR) << "A timeout reached during the dome closing." << sendLog;
		return -2;
	}

	ret = zjisti_stav_portu ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "isClosed: zjisti_stav_portu () error" << sendLog;
		return ret;	// error
	}

	if (getPortState (KONCAK_ZAVRENI) && timeToMoveMin < time (NULL))
		return -2;	// dome is closed

	return USEC_SEC;
}

int D50::endClose ()
{
	int ret;
	VYP (ZAVIRANI);
	ret = zjisti_stav_portu ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "endClose: zjisti_stav_portu () error" << sendLog;
		return ret;	// error
	}

	timeToMoveMax = 0;
	if (!getPortState (KONCAK_ZAVRENI))
		return -1;	// dome is not closed properly :-(

	return 0;
}

int D50::stop ()
{
	if (getPortState (ZAVIRANI))
	{
		VYP (ZAVIRANI);
		return 0;
	}
	if (getPortState (OTEVIRANI))
	{
		VYP (OTEVIRANI);
		return 0;
	}

	return -1;
}

int D50::getMountState ()
{
	rts2core::Connection *connMount = NULL;
	rts2core::Value *tmpValue = NULL;

	connMount = getOpenConnection (mountDevice);
	if (connMount == NULL)
	{
		logStream (MESSAGE_ERROR) << "getMountState: device connection error!" << sendLog;
		return -1;
	}
	if (connMount->getConnState () != CONN_AUTH_OK && connMount->getConnState () != CONN_CONNECTED)
	{
		logStream (MESSAGE_ERROR) << "getMountState: device connection not ready!" << sendLog;
		return -1;
	}

	tmpValue = connMount->getValue ("infotime");
	if (tmpValue == NULL)
	{
		logStream (MESSAGE_ERROR) << "getMountState: infotime is NULL..." << sendLog;
		return -1;
	}
	if (getNow () - tmpValue->getValueDouble () > 10.0)
	{
		logStream (MESSAGE_ERROR) << "getMountState: infotime too old: " << getNow () - tmpValue->getValueDouble () << "s" << sendLog;
		return -1;
	}

	tmpValue = connMount->getValue (mountInsideDomeVariable);
	if (tmpValue == NULL)
	{
		logStream (MESSAGE_ERROR) << "getMountState: wrong mountInsideDomeVariable variable name?" << sendLog;
		return -1;
	}
	if (tmpValue->getValueInteger () == 1)
	{
		return 0;	// telescope is in the safe zone, we can close!
	}
	logStream (MESSAGE_DEBUG) << "startClose: getState () & TEL_MASK_MOVING: " << std::hex << (connMount->getState () & TEL_MASK_MOVING) << sendLog;
	if ((connMount->getState () & TEL_MASK_MOVING) == TEL_PARKING)
	{
		// telescope is just parking...
		return 1;
	}
	return 2; // telescope in obstructing the safe dome-close, it also is not parking
}

int D50::init ()
{
	int ret = Ford::init ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "init: Ford::init () error" << sendLog;
		return ret;
	}

	// get state
	ret = zjisti_stav_portu ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "init: zjisti_stav_portu () error" << sendLog;
		return -1;
	}

	if (isOn (KONCAK_OTEVRENI) && !isOn (KONCAK_ZAVRENI))
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome is closed");
	else if (!isOn (KONCAK_OTEVRENI) && isOn (KONCAK_ZAVRENI))
		maskState (DOME_DOME_MASK, DOME_OPENED, "dome is open");
	else
		logStream (MESSAGE_ERROR) << "dome is neither open nor closed...?!?!" << sendLog;

	if (getPortState (OTEVIRANI) && getPortState (ZAVIRANI))
	{
		logStream (MESSAGE_ERROR) << "Something is REALLY wrong, according to the ford-device, the motor is now working in both directions! It's better to quit now." << sendLog;
		return -1;
	}
	// TODO: somehow solve a case when motor is moving at this point...? Simply stop it, maybe?

	return 0;
}

int D50::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	// nothing here at the moment...
	return Ford::setValue (old_value, new_value);
}

int D50::info ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "info: zjisti_stav_portu () error" << sendLog;
		return -1;
	}
	sw_state->setValueInteger ((getPortState (KONCAK_OTEVRENI))
		| (getPortState (KONCAK_ZAVRENI) << 1)
		| (getPortState (OTEVIRANI) << 2)
		| (getPortState (ZAVIRANI) << 3));

	if (getPortState (OTEVIRANI))
		domeMotorState->setValueInteger (DOME_MOTOR_STATE_OPENING);
	else if (getPortState (ZAVIRANI))
		domeMotorState->setValueInteger (DOME_MOTOR_STATE_CLOSING);
	else
		domeMotorState->setValueInteger (DOME_MOTOR_STATE_OFF);

	domeMotorStateOpening->setValueInteger (getPortState (OTEVIRANI));
	domeMotorStateClosing->setValueInteger (getPortState (ZAVIRANI));
	domeSwitchOpened->setValueInteger (getPortState (KONCAK_OTEVRENI));
	domeSwitchClosed->setValueInteger (getPortState (KONCAK_ZAVRENI));

	// to be independent on teld info period, we ask for it...
	if (!ignoreMountState->getValueBool ())
	{
		rts2core::Connection *connMount = NULL;

		connMount = getOpenConnection (mountDevice);
		if (connMount == NULL)
		{
			logStream (MESSAGE_ERROR) << "info (): mount connection error!" << sendLog;
		}
		else
			connMount->queCommand (new rts2core::Command (this, COMMAND_INFO));
	}

	return Ford::info ();
}

int D50::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_MOUNT && in_addr->isAddress (mountDevice)) {
		logStream (MESSAGE_DEBUG) << "D50::willConnect to DEVICE_TYPE_MOUNT: "<< mountDevice << sendLog;
		return 1;
	}
	return Ford::willConnect (in_addr);
}

int D50::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_MOUNT_DEVICE:
			mountDevice = optarg;
			break;
		case OPT_MOUNT_INDOME_VARIABLE:
			mountInsideDomeVariable = optarg;
			break;
		default:
			return Ford::processOption (in_opt);
	}
	return 0;
}

int main (int argc, char **argv)
{
	D50 device (argc, argv);
	return device.run ();
}
