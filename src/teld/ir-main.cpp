/* 
 * Driver for OpenTPL mounts.
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

#include "ir.h"

#define BLIND_SIZE            1.0
#define OPT_ROTATOR_OFFSET    OPT_LOCAL + 1

class Rts2DevTelescopeIr:public Rts2TelescopeIr
{
	private:
		struct ln_equ_posn target;
		int irTracking;

		double derOff;

		int startMoveReal (double ra, double dec);
	protected:
		virtual int processOption (int in_opt);

		virtual int initValues ();
	public:
		Rts2DevTelescopeIr (int in_arcg, char **in_argv);
		virtual int startMove ();
		virtual int isMoving ();
		virtual int stopMove ();

		virtual int stopWorm ();
		virtual int startWorm ();

		virtual int startPark ();
		int moveCheck (bool park);
		virtual int isParking ();
		virtual int endPark ();

		virtual int changeMasterState (int new_state);
};

int
Rts2DevTelescopeIr::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 't':
			irTracking = atoi (optarg);
			break;
		case OPT_ROTATOR_OFFSET:
			derOff = atof (optarg);
			break;
		default:
			return Rts2TelescopeIr::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevTelescopeIr::initValues ()
{
	int ret;
	ret = Rts2TelescopeIr::initValues ();
	if (ret)
		return ret;
	if (derotatorOffset)
		derotatorOffset->setValueDouble (derOff);

	return Rts2Device::initValues ();
}


int
Rts2DevTelescopeIr::startMoveReal (double ra, double dec)
{
	int status = TPL_OK;
	//  status = setTelescopeTrack (0);

	status = irConn->tpl_set ("POINTING.TARGET.RA", ra / 15.0, &status);
	status = irConn->tpl_set ("POINTING.TARGET.DEC", dec, &status);
	if (derotatorOffset && !getDerotatorPower ())
	{
		status = irConn->tpl_set ("DEROTATOR[3].POWER", 1, &status);
		status = irConn->tpl_set ("CABINET.POWER", 1, &status);
	}

	double offset;

	// apply corrections
	switch (getPointingModel ())
	{
		case 0:
			offset = -1.0 * getCorrRa ();
			status = irConn->tpl_set ("HA.OFFSET", offset, &status);
			offset = -1.0 * getCorrDec ();
			status = irConn->tpl_set ("DEC.OFFSET", offset, &status);
			break;
		case 1:
			offset = getCorrZd ();
			status = irConn->tpl_set ("ZD.OFFSET", offset, &status);
			offset = getCorrAz ();
			status = irConn->tpl_set ("AZ.OFFSET", offset, &status);
			break;

	}

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "IR startMove TRACK status " << status <<
		sendLog;
	#endif

	if (derotatorOffset)
	{
		status = irConn->tpl_set ("DEROTATOR[3].OFFSET", -1 * derotatorOffset->getValueDouble () , &status);

		if (status != TPL_OK)
			return status;
	}

	//  usleep (USEC_SEC);
	status = setTelescopeTrack (irTracking);
	usleep (USEC_SEC);
	return status;
}


int
Rts2DevTelescopeIr::startMove ()
{
	int status = 0;
	double sep;

	struct ln_hrz_posn tAltAz;

	getTarget (&target);

	switch (getPointingModel ())
	{
		case 0:
			getTargetAltAz (&tAltAz);
			if ((tAltAz.az < 5 || tAltAz.az > 355 || (tAltAz.az > 175 && tAltAz.az < 185))
				&& tAltAz.alt < 15)
			{
				logStream (MESSAGE_ERROR) << "Cannot move to target, as it's too close to dome" << sendLog;
				return -1;
			}
			break;
		case 1:
			// move to zenit - move to different dec instead
			if (fabs (target.dec - telLatitude->getValueDouble ()) <= BLIND_SIZE)
			{
				if (fabs (target.ra / 15.0 - getLocSidTime ()) <= BLIND_SIZE / 15.0)
				{
					target.dec = telLatitude->getValueDouble () - BLIND_SIZE;
				}
			}

			double az_off = 0;
			double alt_off = 0;
			status = irConn->tpl_set ("AZ.OFFSET", az_off, &status);
			status = irConn->tpl_set ("ZD.OFFSET", alt_off, &status);
			if (status)
			{
				logStream (MESSAGE_ERROR) << "IR startMove cannot zero offset" << sendLog;
				return -1;
			}
			break;
	}

	status = startMoveReal (target.ra, target.dec);
	if (status)
		return -1;

	// wait till we get it processed
	status = irConn->tpl_get ("POINTING.TARGETDISTANCE", sep, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "cannot get target separation" << sendLog;
		return -1;
	}
	if (sep > 2)
		sleep (3);
	else if (sep > 2 / 60.0)
		usleep (USEC_SEC / 10);
	else
		usleep (USEC_SEC / 10);

	time (&timeout);
	timeout += 30;
	return 0;
}


int
Rts2DevTelescopeIr::isMoving ()
{
	return moveCheck (false);
}


int
Rts2DevTelescopeIr::stopMove ()
{
	int status = 0;
	double zd;
	info ();
	// ZD check..
	status = irConn->tpl_get ("ZD.CURRPOS", zd, &status);
	if (status)
	{
		logStream (MESSAGE_DEBUG) << "IR stopMove cannot get ZD! (" << status <<
			")" << sendLog;
		return -1;
	}
	if (fabs (zd) < 1)
	{
		logStream (MESSAGE_DEBUG) << "IR stopMove suspicious ZD.. " << zd <<
			sendLog;
		status = setTelescopeTrack (0);
		if (status)
		{
			logStream (MESSAGE_DEBUG) << "IR stopMove cannot set track: " <<
				status << sendLog;
			return -1;
		}
		return 0;
	}
	return 0;
}


int
Rts2DevTelescopeIr::startPark ()
{
	int status = TPL_OK;
	// Park to south+zenith
	status = setTelescopeTrack (0);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "IR startPark tracking status " << status <<
		sendLog;
	#endif
	sleep (1);
	status = TPL_OK;
	switch (getPointingModel ())
	{
		case 0:
			status = irConn->tpl_set ("HA.TARGETPOS", 90, &status);
			status = irConn->tpl_set ("DEC.TARGETPOS", 0, &status);
			break;
		case 1:
			status = irConn->tpl_set ("AZ.TARGETPOS", 0, &status);
			status = irConn->tpl_set ("ZD.TARGETPOS", 0, &status);
			status = irConn->tpl_set ("DEROTATOR[3].POWER", 0, &status);
			break;
	}
	if (status)
	{
		logStream (MESSAGE_ERROR) << "IR startPark status " <<
			status << sendLog;
		return -1;
	}
	time (&timeout);
	timeout += 300;
	return 0;
}


int
Rts2DevTelescopeIr::moveCheck (bool park)
{
	int status = TPL_OK;
	int track;
	double poin_dist;
	time_t now;
	struct ln_equ_posn tPos;
	struct ln_equ_posn cPos;
	// if parking, check is different for EQU telescope
	if (park)
	{
		switch (getPointingModel ())
		{
			case 0:
				status = irConn->tpl_get ("HA.TARGETPOS", tPos.ra, &status);
				status = irConn->tpl_get ("DEC.TARGETPOS", tPos.dec, &status);
				status = irConn->tpl_get ("HA.CURRPOS", cPos.ra, &status);
				status = irConn->tpl_get ("DEC.CURRPOS", cPos.dec, &status);
				break;
			case 1:
				status = irConn->tpl_get ("ZD.TARGETPOS", tPos.ra, &status);
				status = irConn->tpl_get ("AZ.TARGETPOS", tPos.dec, &status);
				status = irConn->tpl_get ("ZD.CURRPOS", cPos.ra, &status);
				status = irConn->tpl_get ("AZ.CURRPOS", cPos.dec, &status);
				break;
		}
	}
	else
	{
		// status = irConn->tpl_get ("POINTING.TARGETDISTANCE", poin_dist, &status);
		status = irConn->tpl_get ("POINTING.TARGET.RA", tPos.ra, &status);
		tPos.ra *= 15.0;
		status = irConn->tpl_get ("POINTING.TARGET.DEC", tPos.dec, &status);
		status = irConn->tpl_get ("POINTING.CURRENT.RA", cPos.ra, &status);
		cPos.ra *= 15.0;
		status = irConn->tpl_get ("POINTING.CURRENT.DEC", cPos.dec, &status);
		// for EQU models, we must include target offset
		if (getPointingModel () == 0)
		{
			double haOff, decOff;
			status = irConn->tpl_get ("HA.OFFSET", haOff, &status);
			status = irConn->tpl_get ("DEC.OFFSET", decOff, &status);
			tPos.ra -= haOff;
			tPos.dec -= decOff;
		}
	}
	if (status != TPL_OK)
		return -1;
	poin_dist = ln_get_angular_separation (&cPos, &tPos);
	time (&now);
	// get track..
	status = irConn->tpl_get ("POINTING.TRACK", track, &status);
	if (track == 0 && !park)
	{
		logStream (MESSAGE_WARNING) <<
			"Tracking sudently stopped, reenable tracking (track=" << track << " park = " << park << ")" << sendLog;
		setTelescopeTrack (irTracking);
		sleep (1);
		return USEC_SEC / 100;
	}
	// 0.01 = 36 arcsec
	if (fabs (poin_dist) <= 0.01)
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "IR isMoving target distance " << poin_dist
			<< sendLog;
		#endif
		return -2;
	}
	// finish due to timeout
	if (timeout < now)
	{
		logStream (MESSAGE_ERROR) << "IR isMoving target distance in timeout "
			<< poin_dist << "(" << status << ")" << sendLog;
		return -1;
	}
	return USEC_SEC / 100;
}


int
Rts2DevTelescopeIr::isParking ()
{
	return moveCheck (true);
}


int
Rts2DevTelescopeIr::endPark ()
{
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "IR endPark" << sendLog;
	#endif
	return 0;
}


Rts2DevTelescopeIr::Rts2DevTelescopeIr (int in_argc, char **in_argv):Rts2TelescopeIr (in_argc,
in_argv)
{
	irTracking = 4;

	derOff = 0;

	addOption (OPT_ROTATOR_OFFSET, "rotator_offset", 1, "rotator offset, default to 0");
	addOption ('t', "ir_tracking", 1,
		"IR tracking (1, 2, 3 or 4 - read OpenTCI doc; default 4");
}


int
Rts2DevTelescopeIr::startWorm ()
{
	int status = TPL_OK;
	status = setTelescopeTrack (irTracking);
	if (status != TPL_OK)
		return -1;
	return 0;
}


int
Rts2DevTelescopeIr::stopWorm ()
{
	int status = TPL_OK;
	status = setTelescopeTrack (0);
	if (status)
		return -1;
	return 0;
}


int
Rts2DevTelescopeIr::changeMasterState (int new_state)
{
	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			coverOpen ();
			break;
		default:
			coverClose ();
			break;
	}
	return Rts2DevTelescope::changeMasterState (new_state);
}


int
main (int argc, char **argv)
{
	Rts2DevTelescopeIr device = Rts2DevTelescopeIr (argc, argv);
	return device.run ();
}
