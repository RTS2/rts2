/* 
 * INDI bridge.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/device.h"
#include "../utils/rts2command.h"

#include "indidevapi.h"
#include "indidrivermain.h"
#include "indicom.h"

#define RTS2_GROUP  "RTS2"
#define BASIC_GROUP "Main Control"

#define mydev       "RTS2"

#define POLLMS      10		 /* poll period, ms */

/**
 * Class which acts as bridge between INDI and RTS2.
 * 
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Indi:public rts2core::Device
{
	protected:
		const char *telescopeName;

		virtual int willConnect (Rts2Address * in_addr);

		virtual int init ();

		virtual int processOption (int opt);
	public:
		Indi (int argc, char **argv);
		virtual ~ Indi (void);

		virtual int changeMasterState (int new_state);

		void setStates ();
		void setObjRaDec (double ra, double dec);
		void setCorrRaDec (double ra, double dec);
		void ISPoll ();

		virtual void message (Rts2Message & msg);
};

int
Indi::willConnect (Rts2Address * in_addr)
{
	return 1;
}


Indi::Indi (int in_argc, char **in_argv):
rts2core::Device (in_argc, in_argv, DEVICE_TYPE_INDI, "INDI")
{
	telescopeName = "T0";
	setLockPrefix ("/tmp/rts2_");
	setNotDaemonize ();
	IDLog ("Initializing Indi");

	addOption ('t', NULL, 1, "telescope name (default to T0)");
}


Indi::~Indi (void)
{

}

int Indi::init ()
{
	int ret;
	ret = rts2core::Device::init ();
	if (ret)
		return ret;

	setMessageMask (MESSAGE_MASK_ALL);
	return 0;
}

int Indi::processOption (int opt)
{
	switch (opt)
	{
		case 't':
			telescopeName = optarg;
			break;
		default:
			return rts2core::Device::processOption (opt);
	}
	return 0;
}

void Indi::message (Rts2Message & msg)
{
	IDMessage (mydev, "%s %s", msg.getMessageOName (), msg.getMessageString ());
}


/*INDI controls */
static ISwitch PowerS[] =
{
	{"CONNECT", "Connect", ISS_OFF, 0, 0},
	{"DISCONNECT", "Disconnect", ISS_ON, 0, 0}
};

ISwitchVectorProperty PowerSP = {mydev, "CONNECTION", "Connection", BASIC_GROUP, IP_RW, ISR_1OFMANY, 0,	IPS_IDLE, PowerS, NARRAY (PowerS), 0, 0};

static ISwitch StatesS[] =
{
 	{"OFF", "Off", ISS_ON, 0, 0},
	{"STANDBY", "Standby", ISS_OFF, 0, 0},
	{"ON", "On", ISS_OFF, 0, 0}
};

ISwitchVectorProperty StatesSP =
{mydev, "STATE", "State", RTS2_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE, StatesS, NARRAY (StatesS), 0, 0};

static INumber eq[] =
{
	{"RA", "RA  H:M:S", "%10.6m", 0., 24., 0., 0., 0, 0, 0},
	{"DEC", "Dec D:M:S", "%10.6m", -90., 90., 0., 0., 0, 0, 0},
};

INumberVectorProperty eqNum =
{mydev, "EQUATORIAL_COORD", "Equatorial J2000", BASIC_GROUP, IP_RW, 0, IPS_IDLE, eq, NARRAY (eq), 0, 0};

static INumber offs[] =
{
	{"RA", "RA  H:M:S", "%10.6m", -24., 24., 0., 0., 0, 0, 0},
	{"DEC", "Dec D:M:S", "%10.6m", -90., 90., 0., 0., 0, 0, 0},
};

INumberVectorProperty eqOffsets =
{mydev, "EQUATORIAL_OFFSET", "Equatorial offset", BASIC_GROUP, IP_RW, 0, IPS_IDLE, eq, NARRAY (offs), 0, 0};

INumberVectorProperty eqCorr =
{mydev, "EQUATORIAL_CORRECTION", "Equatorial correction", BASIC_GROUP, IP_RW, 0, IPS_IDLE, eq, NARRAY (offs), 0, 0};

static INumber hor[] =
{
	{"ALT", "Alt  D:M:S", "%10.6m", -90., 90., 0., 0., 0, 0, 0},
	{"AZ", "Az D:M:S", "%10.6m", 0., 360., 0., 0., 0, 0, 0}
};

static INumberVectorProperty horNum =
{mydev, "HORIZONTAL_COORD", "Horizontal Coords", BASIC_GROUP, IP_RW, 0, IPS_IDLE, hor, NARRAY (hor), 0, 0};

static ISwitch OnCoordSetS[] =
{
	{"SLEW", "Slew", ISS_ON, 0, 0},
	{"TRACK", "Track", ISS_OFF, 0, 0},
	{"SYNC", "Sync", ISS_OFF, 0, 0}
};

static ISwitchVectorProperty OnCoordSetSw =
{mydev, "ON_COORD_SET", "On Set", BASIC_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE, OnCoordSetS, NARRAY (OnCoordSetS), 0, 0};

static ISwitch abortSlewS[] =
{
	{"ABORT", "Abort", ISS_OFF, 0, 0}
};

static ISwitchVectorProperty abortSlewSw =
{mydev, "ABORT_MOTION", "Abort Slew/Track", BASIC_GROUP, IP_RW,	ISR_1OFMANY, 0, IPS_IDLE, abortSlewS, NARRAY (abortSlewS), 0, 0};

static Indi *device = NULL;

void Indi::setStates ()
{
	if (StatesSP.sp[0].s == ISS_ON)
	{
	  	sendMasters ("off");
		StatesSP.s = IPS_BUSY;
		IDSetSwitch (&StatesSP, "System is switched to off, will not observe.");
	}
	else if (StatesSP.sp[1].s == ISS_ON)
	{
	  	sendMasters ("standby");
		StatesSP.s = IPS_BUSY;
		IDSetSwitch (&StatesSP,	"System is switched to standby, will not observe.");
	}
	else if (StatesSP.sp[2].s == ISS_ON)
	{
		sendMasters ("on");
		StatesSP.s = IPS_BUSY;
		IDSetSwitch (&StatesSP, "System is switched to on, will observe.");
	}
}

void Indi::setObjRaDec (double ra, double dec)
{
	Rts2Conn *tel = getOpenConnection (telescopeName);
	if (tel)
	{
		tel->queCommand (new rts2core::Rts2CommandResyncMove (this, (rts2core::Rts2DevClientTelescope *)tel->getOtherDevClient (), ra, dec));
	}
}

void Indi::setCorrRaDec (double ra, double dec)
{
	Rts2Conn *tel = getOpenConnection (telescopeName);
	if (tel)
	{
		tel->queCommand (new rts2core::Rts2CommandChangeValue ((rts2core::Rts2DevClientTelescope *)tel->getOtherDevClient (), "CORR_", '=', ra, dec));
	}
}

int Indi::changeMasterState (int new_state)
{
	StatesSP.s = IPS_BUSY;
	IDSetSwitch (&StatesSP, NULL);

	StatesS[0].s = ISS_OFF;
	StatesS[1].s = ISS_OFF;
	StatesS[2].s = ISS_OFF;

	if (new_state == SERVERD_HARD_OFF)
		StatesS[0].s = ISS_ON;
	else if (new_state & SERVERD_STANDBY_MASK)
		StatesS[1].s = ISS_ON;
	else
		StatesS[2].s = ISS_ON;

	StatesSP.s = IPS_OK;
	IDSetSwitch (&StatesSP, "Changed RTS2 state from other source");

	return rts2core::Device::changeMasterState (new_state);
}

void Indi::ISPoll ()
{
	oneRunLoop ();

	Rts2Conn *tel = getOpenConnection (telescopeName);
	if (tel)
	{
		rts2core::Value *val = tel->getValue ("OBJ");
		if (val && val->getValueBaseType () == RTS2_VALUE_RADEC)
		{
			eqNum.np[0].value = ((rts2core::ValueRaDec *) val)->getRa () / 15.0;
			eqNum.np[1].value = ((rts2core::ValueRaDec *) val)->getDec ();
			eqNum.s = IPS_OK;
		}
		else
		{
		  	eqNum.s = IPS_BUSY;
		}
		IDSetNumber (&eqNum, NULL);

		val = tel->getValue ("OFFS");
		if (val && val->getValueBaseType () == RTS2_VALUE_RADEC)
		{
			eqOffsets.np[0].value = ((rts2core::ValueRaDec *) val)->getRa () / 15.0;
			eqOffsets.np[1].value = ((rts2core::ValueRaDec *) val)->getDec ();
			eqOffsets.s = IPS_OK;
		}
		else
		{
		  	eqOffsets.s = IPS_BUSY;
		}
		IDSetNumber (&eqOffsets, NULL);

		val = tel->getValue ("CORR_");
		if (val && val->getValueBaseType () == RTS2_VALUE_RADEC)
		{
			eqCorr.np[0].value = ((rts2core::ValueRaDec *) val)->getRa () / 15.0;
			eqCorr.np[1].value = ((rts2core::ValueRaDec *) val)->getDec ();
			eqCorr.s = IPS_OK;
		}
		else
		{
		  	eqCorr.s = IPS_BUSY;
		}
		IDSetNumber (&eqCorr, NULL);

		val = tel->getValue ("TEL_");
		if (val && val->getValueBaseType () == RTS2_VALUE_ALTAZ)
		{
			horNum.np[0].value = ((rts2core::ValueAltAz *) val)->getAlt ();
			horNum.np[1].value = ((rts2core::ValueAltAz *) val)->getAz ();
			horNum.s = IPS_OK;
		}
		else
		{
		  	horNum.s = IPS_BUSY;
		}
		IDSetNumber (&horNum, NULL);

		PowerS[0].s = ISS_ON;
		PowerS[1].s = ISS_OFF;
		PowerSP.s = IPS_OK;
		IDSetSwitch (&PowerSP, NULL);
	}
}


/**
 * That's INDI specific part..
 */
void ISInit ()
{
	if (device)
		return;
	device->initDaemon ();
}

void ISPoll (void *p)
{
	ISInit ();
	device->ISPoll ();
	IEAddTimer (POLLMS, ISPoll, NULL);
}

void ISGetProperties (const char *dev)
{
	ISInit ();

	IDDefSwitch (&PowerSP, NULL);
	IDDefSwitch (&StatesSP, NULL);
	IDDefNumber (&eqNum, NULL);
	IDDefNumber (&eqOffsets, NULL);
	IDDefNumber (&eqCorr, NULL);
	IDDefNumber (&horNum, NULL);

	IDDefSwitch (&OnCoordSetSw, NULL);
	IDDefSwitch (&abortSlewSw, NULL);

	IEAddTimer (POLLMS, ISPoll, NULL);
}

void ISNewSwitch (const char *dev, const char *name, ISState * states, char *names[], int n)
{
	ISInit ();
	if (!strcmp (name, StatesSP.name))
	{
		device->setStates ();
		return;
	}
}

void ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
	ISInit ();
}

void ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	ISInit ();

	double newRA;
	double newDEC;

	int i=0, nset=0;

	if (!strcmp (name, eqNum.name))
	{
		// parse move request
		for (nset = i = 0; i < n; i++)
		{
			INumber *eqp = IUFindNumber (&eqNum, names[i]);
			if (eqp == &eq[0])
			{
        	        	newRA = values[i];
				nset += newRA >= 0 && newRA <= 24.0;
			}
			else if (eqp == &eq[1])
			{
				newDEC = values[i];
				nset += newDEC >= -90.0 && newDEC <= 90.0;
			}
		}

		if (nset == 2)
		{
			eqNum.s = IPS_BUSY;
			device->setObjRaDec (newRA * 15.0, newDEC);
			eqNum.s = IPS_IDLE;
			IDSetNumber(&eqNum, NULL);
			return;
		}
		else
		{
			eqNum.s = IPS_IDLE;
			IDSetNumber(&eqNum, "RA or Dec missing or invalid");
		}
	}
	else if (!strcmp (name, eqCorr.name))
	{
		// parse move request
		for (nset = i = 0; i < n; i++)
		{
			INumber *eqp = IUFindNumber (&eqCorr, names[i]);
			if (eqp == &eq[0])
			{
        	        	newRA = values[i];
				nset += newRA >= 0 && newRA <= 24.0;
			}
			else if (eqp == &eq[1])
			{
				newDEC = values[i];
				nset += newDEC >= -90.0 && newDEC <= 90.0;
			}
		}

		if (nset == 2)
		{
			eqCorr.s = IPS_BUSY;
			device->setCorrRaDec (newRA * 15.0, newDEC);
			eqCorr.s = IPS_IDLE;
			IDSetNumber(&eqCorr, NULL);
			return;
		}
		else
		{
			eqCorr.s = IPS_IDLE;
			IDSetNumber(&eqCorr, "RA or Dec missing or invalid");
		}
	}
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	ISInit ();
}

void ISSnoopDevice (XMLEle *root)
{
	ISInit ();
}

int main (int ac, char **av)
{
	device = new Indi (ac, av);
	std::cerr << "new" << std::endl;
	device->initDaemon ();
	IDProcessParams (ac, av);
	return IDMain ();
}
