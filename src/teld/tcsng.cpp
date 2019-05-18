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

#include <cmath>

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

		virtual int setTracking(int track, bool addTrackingTimer = false, bool send = true);

		virtual int moveTLE (const char *l1, const char *l2);
		
	private:
		rts2core::ConnTCSNG *ngconn;

		HostString *host;
		const char *cfgFile;
		const char *ngname;
		double waitMove;

		rts2core::ValueBool *systemEnable;
		rts2core::ValueBool *domeAuto;
		rts2core::ValueBool *domeInit;
		rts2core::ValueBool *domeHome;
		rts2core::ValueBool *pecState;
		rts2core::ValueDouble *domeAz;
		rts2core::ValueSelection *tcsngmoveState;
		rts2core::ValueInteger *motionState;
		rts2core::ValueDouble *settle_time;

		rts2core::ValueString *tle_l1;
		rts2core::ValueString *tle_l2;
		rts2core::ValueInteger *tle_ephem; 
		rts2core::ValueDouble *tle_distance;
		rts2core::ValueBool *tle_freeze;
		rts2core::ValueDouble *tle_rho_sin_phi;
		rts2core::ValueDouble *tle_rho_cos_phi;

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

	createValue (pecState, "pec_state", "PEC", false, RTS2_VALUE_WRITABLE);
	pecState->setValueBool (false);
	
	
	/*RTS2 moveState is an rts2 data type that is alwasy 
	set equal to the tcsng.moveState member 
	this way moveState variable can be seen in rts2-mon*/
	createValue (tcsngmoveState, "tcsng_move_state", "TCSNG move state", false);
	tcsngmoveState->addSelVal ("NO MOVE");
	tcsngmoveState->addSelVal ("MOVE CALLED");
	tcsngmoveState->addSelVal ("MOVING");
	tcsngmoveState->setValueInteger (0);

	createValue (motionState, "motion", "TCSNG motion variable", false, RTS2_DT_HEX);

	createValue (settle_time, "settle_time", "time when telescope shall be on position before ending the move", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE);
	settle_time->setValueDouble (0.5);

	createValue (reqcount, "tcsng_req_count", "TCSNG request counter", false);
	reqcount->setValueInteger (TCSNG_NO_MOVE_CALLED);
	
	/*
	createValue (tle_l1, "tle_l1_target", "TLE target line 1", false);
	createValue (tle_l2, "tle_l2_target", "TLE target line 2", false);
	createValue (tle_ephem, "tle_ephem", "TLE emphemeris type", false);
	createValue (tle_distance, "tle_distance", "[km] satellite distance", false);
	createValue (tle_freeze, "tle_freeze", "if true, stop updating TLE positions; put current speed vector to DRATE", false, RTS2_VALUE_WRITABLE);
	createValue (tle_rho_sin_phi, "tle_rho_sin", "TLE rho_sin_phi (observatory position)", false); 	
	createValue (tle_rho_cos_phi, "tle_rho_cos", "TLE rho_cos_phi (observatory position)", false); 
	*/

	ngconn = NULL;
	host = NULL;
	cfgFile = NULL;
	ngname = NULL;

	addOption (OPT_CONFIG, "config", 1, "configuration file");
	addOption ('t', NULL, 1, "TCS NG hostname[:port]");
	addOption ('n', NULL, 1, "TCS NG telescope device name");
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
		case 'n':
			ngname = optarg;
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

	if (ngname == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify TCS NG device name (with -n option)." << sendLog;
		return -1;
	}


	ngconn = new rts2core::ConnTCSNG (this, host->getHostname (), host->getPort (), ngname, "TCS");
	ngconn->setDebug (getDebug ());

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile (cfgFile);
	setTelLongLat (config->getObserver ()->lng, config->getObserver ()->lat);
	setTelAltitude (config->getObservatoryAltitude ());
			
	return Telescope::initHardware ();
}

int TCSNG::info ()
{
	double nglst;
	try
	{
		setTelRaDec (ngconn->getSexadecimalHours ("RA"), ngconn->getSexadecimalAngle ("DEC"));
		nglst = ngconn->getSexadecimalTime ("ST");

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
		const char * pecst = ngconn->request("PECSTAT");
		int pecCond, pecCount, pecIndex, pecMode;
		slen = sscanf (pecst, "%d %d %d %d", &pecCond, &pecCount, &pecIndex, &pecMode);
		if (slen == 4)
		{
			pecState->setValueBool( pecCond == 1 || pecCond == 3 );
			if( pecCond == 3 )
			logStream(MESSAGE_WARNING) << "PEC has not yet found the index." << sendLog;
		}
		reqcount->setValueInteger (ngconn->getReqCount ());
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "info " << er << sendLog;
		delete ngconn;
		sleep(1);
		ngconn = new rts2core::ConnTCSNG (this, host->getHostname (), host->getPort (), ngname, "TCS");
		return -1;
	}

	return Telescope::infoLST (nglst);
}

int TCSNG::startResync ()
{
	struct ln_equ_posn tar;
	getTarget (&tar);

	waitMove = NAN;

	char cmd[200];
	ngconn->command("CANCEL");
	snprintf (cmd, 200, "NEXTPOS %s %s 2000 0 0", deg2hours (tar.ra), deg2dec (tar.dec));
	ngconn->command (cmd);
	ngconn->command ("MOVNEXT");

	tcsngmoveState->setValueInteger (TCSNG_MOVE_CALLED);
  	return 0;
}

int TCSNG::startMoveFixed (double tar_az, double tar_alt)
{
	char cmd[200];

	waitMove = NAN;
	snprintf (cmd, 200, "ELAZ %lf %lf", tar_alt, tar_az);

	ngconn->command (cmd);
	return 0;
}

int TCSNG::stopMove ()
{
	waitMove = NAN;
	ngconn->command ("CANCEL");
	tcsngmoveState->setValueInteger (TCSNG_NO_MOVE_CALLED);
	return 0;
}

int TCSNG::startPark ()
{
	waitMove = NAN;
	ngconn->command ("MOVSTOW");
	tcsngmoveState->setValueInteger (TCSNG_MOVE_CALLED);
	return 0;
}

int TCSNG::isMoving ()
{
	int mot = 0;
	try
	{
		mot = atoi (ngconn->request ("MOTION"));
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "isMoving " << er << sendLog;
		return -1;
	}
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
			{
				if (!std::isnan(waitMove))
				{
					logStream(MESSAGE_WARNING) << "telescope moved during settle time" << sendLog;
					waitMove = NAN;
				}
				return USEC_SEC / 100;
			}
			// check the counter..
			if (std::isnan(waitMove))
			{
				if (std::isnan (settle_time->getValueDouble()) || settle_time->getValueDouble() <= 0)
					return -2;
				logStream(MESSAGE_INFO) << "starting settle period " << TimeDiff(settle_time->getValueDouble()) << sendLog;
				waitMove = getNow() + settle_time->getValueDouble();
				return USEC_SEC / 100;
			}
			if (waitMove < getNow())
			{
				waitMove = NAN;
				tcsngmoveState->setValueInteger (TCSNG_NO_MOVE_CALLED);
				return -2;
			}
			return USEC_SEC / 100;
	}
	return -1;
}

int TCSNG::setTracking( int track, bool addTrackingTimer, bool send)
{
	return Telescope::setTracking (track, addTrackingTimer, send);
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

	if (oldValue == pecState)
	{
		ngconn->command (((rts2core::ValueBool *) newValue)->getValueBool () ? "PEC ON" : "PEC OFF");
		return 0;
	}

	return Telescope::setValue (oldValue, newValue);
}

int TCSNG::moveTLE (const char *l1, const char *l2)
{
	char cmd[300];

	logStream (MESSAGE_INFO) << "TLE incoming" << l1 << " " << l2 << sendLog;

	snprintf (cmd, 300, "TLE\n%s\n%s", l1, l2 );
	ngconn->command( cmd );
	logStream (MESSAGE_INFO) << cmd << sendLog;
	snprintf (cmd, 300, "SATTRACK START");
	ngconn->command(cmd);

	return Telescope::moveTLE (l1, l2);
}

int main (int argc, char **argv)
{
	TCSNG device (argc, argv);
	return device.run ();
}
