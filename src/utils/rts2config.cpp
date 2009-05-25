/* 
 * Configuration file read routines.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include <math.h>
#include <string.h>

#include "rts2config.h"

Rts2Config *Rts2Config::pInstance = NULL;

int
Rts2Config::getSpecialValues ()
{
	int ret = 0;
	std::string horizon_file;

	// get some commonly used values
	ret += getDouble ("observatory", "longitude", observer.lng);
	ret += getDouble ("observatory", "latitude", observer.lat);
	ret += getDouble ("observatory", "altitude", observatoryAltitude, nan("f"));
	// load horizont file..
	getString ("observatory", "horizon", horizon_file, "");

	getStringVector ("observatory", "required_devices", obs_requiredDevices);
	getStringVector ("imgproc", "astrometry_devices", imgproc_astrometryDevices);

	getString ("observatory", "que_path", obs_quePath, "%b/que/%c/%f");
	getString ("observatory", "acq_path", obs_acqPath, "%b/acqusition/%t/%c/%f");
	getString ("observatory", "archive_path", obs_archive, "%b/archive/%t/%c/object/%f");
	getString ("observatory", "trash_path", obs_trash, "%b/trash/%t/%c/%f");
	getString ("observatory", "flat_path", obs_flats, "%b/flat/%c/raw/%f");
	getString ("observatory", "dark_path", obs_darks, "%b/darks/%c/%f");

	getDouble ("observatory", "min_flat_heigh", minFlatHeigh, 10);

	checker = new ObjectCheck (horizon_file.c_str ());
	getInteger ("imgproc", "astrometry_timeout", astrometryTimeout, 3600);
	getDouble ("calibration", "airmass_distance", calibrationAirmassDistance, 0.1);
	getDouble ("calibration", "lunar_dist", calibrationLunarDist, 20);
	getInteger ("calibration", "valid_time", calibrationValidTime, 3600);
	getInteger ("calibration", "max_delay", calibrationMaxDelay, 7200);
	getFloat ("calibration", "min_bonus", calibrationMinBonus, 1.0);
	getFloat ("calibration", "max_bonus", calibrationMaxBonus, 300.0);

	getFloat ("swift", "min_horizon", swift_min_horizon, 0);
	getFloat ("swift", "soft_horizon", swift_soft_horizon, swift_min_horizon);


	// GRD section
	grbd_follow_transients = getBoolean ("grbd", "know_transients", true);
	getInteger ("grbd", "validity", grbd_validity, 3600);

	if (ret)
		return -1;

	return Rts2ConfigRaw::getSpecialValues ();
}


Rts2Config::Rts2Config ():Rts2ConfigRaw ()
{
	observer.lat = 0;
	observer.lng = 0;
	checker = NULL;
	// default to 120 seconds
	astrometryTimeout = 120;
}


Rts2Config::~Rts2Config (void)
{
	delete checker;
}


Rts2Config *
Rts2Config::instance ()
{
	if (!pInstance)
		pInstance = new Rts2Config ();
	return pInstance;
}


struct ln_lnlat_posn *
Rts2Config::getObserver ()
{
	return &observer;
}


ObjectCheck *
Rts2Config::getObjectChecker ()
{
	return checker;
}


int
Rts2Config::getDeviceMinFlux (const char *device, double &minFlux)
{
	return getDouble (device, "minflux", minFlux);
}


time_t
Rts2Config::getNight ()
{
	time_t now = getNight (time (NULL));
	struct tm *tm_s = gmtime (&now);
	return getNight (tm_s->tm_year + 1900, tm_s->tm_mon + 1, tm_s->tm_mday);
}

time_t
Rts2Config::getNight (int year, int month, int day)
{
	struct tm _tm;
	static char p_tz[100];

	_tm.tm_year = year - 1900;
	_tm.tm_mon = month - 1;
	_tm.tm_mday = day;
	_tm.tm_hour = 12 - getObservatoryLongitude () / 15;
	_tm.tm_min = _tm.tm_sec = 0;

	std::string old_tz;
	if (getenv("TZ"))
		old_tz = std::string (getenv ("TZ"));

	putenv ((char*) "TZ=UTC");

	time_t n = mktime (&_tm);

	strcpy (p_tz, "TZ=");

	if (old_tz.length () > 0)
	{
		strncat (p_tz, old_tz.c_str (), 96);
	}
	putenv (p_tz);

	return n;
}
