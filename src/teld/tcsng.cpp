/*
 * TCS NG telescope driver.
 * Copyright (C) 2016 Scott Swindel
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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


#include "teld.h"
#include "configuration.h"
#include "connection/tcsng.h"

using namespace rts2teld;

/*moveState: 0=>Not moving no move called, 
                1=>move called but not moving, 2=>moving*/
#define TCSNG_NO_MOVE_CALLED   0
#define TCSNG_MOVE_CALLED      1
#define TCSNG_MOVING           2

/**
 * TCS telescope class to interact with TCSng control systems.
 *
 * author Scott Swindell
 */
class TCSNG:public Telescope
{
	public:
		TCSNG (int argc, char **argv);
		virtual ~TCSNG ();

	protected:
		virtual int processOption (int in_opt);

		virtual int initHardware ();

		virtual int info ();

		virtual int startResync ();

		virtual int startMoveFixed (double tar_az, double tar_alt);

		virtual int stopMove ();

		virtual int startPark ();

		virtual int endPark ()
		{
			return 0;
		}

		virtual int isMoving ();
			
		virtual int isMovingFixed ()
		{
			return isMoving ();
		}

		virtual int isParking ()
		{
			return isMoving ();
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

	private:
		rts2core::ConnTCSNG *ngconn;

		HostString *host;
		const char *cfgFile;

		rts2core::ValueBool *systemEnable;
		rts2core::ValueBool *domeAuto;
		rts2core::ValueBool *domeInit;
		rts2core::ValueBool *domeHome;
		rts2core::ValueDouble *domeAz;
		rts2core::ValueSelection *tcsngmoveState;
		rts2core::ValueInteger *motionState;

		rts2core::ValueInteger *reqcount;
};

using namespace rts2teld;

const char *deg2hours (double h)
{
	static char rbuf[200];
	struct ln_hms hms;
	ln_deg_to_hms (h, &hms);

	snprintf (rbuf, 200, "%02d:%02d:%02.2f", hms.hours, hms.minutes, hms.seconds);
	return rbuf;
}

const char *deg2dec (double d)
{
	static char rbuf[200];
	struct ln_dms dms;
	ln_deg_to_dms (d, &dms);

	snprintf (rbuf, 200, "%c%02d:%02d:%02.2f", dms.neg ? '-':'+', dms.degrees, dms.minutes, dms.seconds);
	return rbuf;
}

TCSNG::TCSNG (int argc, char **argv):Telescope (argc,argv, true, true)
{
	createValue (systemEnable, "system_enable", "system is enabled for observations", false, RTS2_VALUE_WRITABLE);
	systemEnable->setValueBool (false);

	createValue (domeAuto, "dome_auto", "dome follows the telescope", false, RTS2_VALUE_WRITABLE);
	domeAuto->setValueBool (false);

	createValue (domeInit, "dome_init", "dome is ready to work", false);
	domeInit->setValueBool (false);

	createValue (domeHome, "dome_home", "dome is homed", false);
	domeHome->setValueBool (false);

	createValue (domeAz, "dome_az", "dome azimuth", false);
	
	/*RTS2 moveState is an rts2 data type that is alwasy 
	set equal to the tcsng.moveState member 
	this way moveState variable can be seen in rts2-mon*/
	createValue (tcsngmoveState, "tcsng_move_state", "TCSNG move state", false);
	tcsngmoveState->addSelVal ("NO MOVE");
	tcsngmoveState->addSelVal ("MOVE CALLED");
	tcsngmoveState->addSelVal ("MOVING");
	tcsngmoveState->setValueInteger (0);

	createValue (motionState, "motion", "TCSNG motion variable", false, RTS2_DT_HEX);

	createValue (reqcount, "tcsng_req_count", "TCSNG request counter", false);
	reqcount->setValueInteger (TCSNG_NO_MOVE_CALLED);

	ngconn = NULL;
	host = NULL;
	cfgFile = NULL;

	addOption (OPT_CONFIG, "config", 1, "configuration file");
	addOption ('t', NULL, 1, "TCS NG hostname[:port]");
}

TCSNG::~TCSNG ()
{
	delete ngconn;
}

int TCSNG::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_CONFIG:
			cfgFile = optarg;
			break;
		case 't':
			host = new HostString (optarg, "5750");
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int TCSNG::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify TCS NG server hostname (with -t option)." << sendLog;
		return -1;
	}

	ngconn = new rts2core::ConnTCSNG (this, host->getHostname (), host->getPort (), "TCSNG", "TCS");
	ngconn->setDebug (getDebug ());

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile (cfgFile);
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
			
	return Telescope::initHardware ();
}

int TCSNG::info ()
{
	setTelRaDec (ngconn->getSexadecimalHours ("RA"), ngconn->getSexadecimalAngle ("DEC"));
	double nglst = ngconn->getSexadecimalTime ("ST");

	systemEnable->setValueBool (ngconn->getInteger ("DISABLE") == 0);

	const char * domest = ngconn->request ("DOME");
	double del,telaz,az;
	int mod, in, home;
	size_t slen = sscanf (domest, "%lf %d %d %lf %lf %d", &del, &mod, &in, &telaz, &az, &home);
	if (slen == 6)
	{
		domeAuto->setValueBool (mod == 1);
		domeInit->setValueBool (in == 1);
		domeAz->setValueDouble (az);
		domeHome->setValueBool (home == 1);
	}

	reqcount->setValueInteger (ngconn->getReqCount ());

	return Telescope::infoLST (nglst);
}

int TCSNG::startResync ()
{
	struct ln_equ_posn tar;
	getTarget (&tar);

	char cmd[200];
	snprintf (cmd, 200, "NEXTPOS %s %s 2000 0 0", deg2hours (tar.ra), deg2dec (tar.dec));
	ngconn->command (cmd);
	ngconn->command ("MOVNEXT");

	tcsngmoveState->setValueInteger (TCSNG_MOVE_CALLED);
  	return 0;
}

int TCSNG::startMoveFixed (double tar_az, double tar_alt)
{
	char cmd[200];
	snprintf (cmd, 200, "ELAZ %lf %lf", tar_alt, tar_az);

	ngconn->command (cmd);
	return 0;
}

int TCSNG::stopMove ()
{
	ngconn->command ("CANCEL");
	tcsngmoveState->setValueInteger (TCSNG_NO_MOVE_CALLED);
	return 0;
}

int TCSNG::startPark ()
{
	ngconn->command ("MOVSTOW");
	tcsngmoveState->setValueInteger (TCSNG_MOVE_CALLED);
	return 0;
}

int TCSNG::isMoving ()
{
	int mot = atoi (ngconn->request ("MOTION"));
	motionState->setValueInteger (mot);
	switch (tcsngmoveState->getValueInteger ())
	{
		case TCSNG_MOVE_CALLED:
			if ((mot & TCSNG_RA_AZ) || (mot & TCSNG_DEC_AL) || (mot & TCSNG_DOME))
				tcsngmoveState->setValueInteger (TCSNG_MOVING);
			if (getNow () - getTargetStarted () > 5)
				return -2;
			return USEC_SEC / 100;
			break;
		case TCSNG_MOVING:
			if ((mot & TCSNG_RA_AZ) || (mot & TCSNG_DEC_AL) || (mot & TCSNG_DOME))
				return USEC_SEC / 100;
			tcsngmoveState->setValueInteger (TCSNG_NO_MOVE_CALLED);
			return -2;
			break;
	}
	return -1;
}

int TCSNG::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == domeAuto)
	{
		ngconn->command (((rts2core::ValueBool *) newValue)->getValueBool () ? "DOME AUTO ON" : "DOME AUTO OFF");
		return 0;
	}
	if (oldValue == systemEnable)
	{
		ngconn->command (((rts2core::ValueBool *) newValue)->getValueBool () ? "ENABLE" : "DISABLE");
		return 0;
	}
	return Telescope::setValue (oldValue, newValue);
}

int main (int argc, char **argv)
{
	TCSNG device (argc, argv);
	return device.run ();
}
