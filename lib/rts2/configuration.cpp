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

#include "configuration.h"

using namespace rts2core;

Configuration *Configuration::pInstance = NULL;

int Configuration::getSpecialValues ()
{
	int ret = 0;
	std::string horizon_file;

	// get some commonly used values
	ret += getDouble ("observatory", "longitude", observer.lng);
	ret += getDouble ("observatory", "latitude", observer.lat);
	ret += getDouble ("observatory", "altitude", observatoryAltitude);

	storeSexadecimals = getBoolean ("observatory", "sexadecimals", false);

	// load horizon file..
	getString ("observatory", "horizon", horizon_file, "-");

	// load UT offset
	getFloat ("observatory", "ut_offset", utOffset, observer.lng / 15.0);

	getStringVector ("observatory", "required_devices", obs_requiredDevices, false);
	getStringVector ("imgproc", "astrometry_devices", imgproc_astrometryDevices, false);

	getString ("observatory", "base_path", obs_basePath, "/images/");
	getString ("observatory", "que_path", obs_quePath, "%b/queue/%c/%f");
	getString ("observatory", "acq_path", obs_acqPath, "");
	getString ("observatory", "archive_path", obs_archive, "");
	getString ("observatory", "trash_path", obs_trash, "");
	getString ("observatory", "flat_path", obs_flats, "");
	getString ("observatory", "dark_path", obs_darks, "");

	getString ("observatory", "target_path", targetDir, RTS2_PREFIX "/etc/rts2/targets");
	masterConsFile = targetDir + "/constraints.xml";

	getString ("observatory", "nightlogs", nightDir, RTS2_PREFIX "/etc/rts2/nights/%N.fits");

	minFlatHeigh = getDoubleDefault ("observatory", "min_flat_heigh", 10);

	checker = new ObjectCheck (horizon_file.c_str ());
	astrometryTimeout = getIntegerDefault ("imgproc", "astrometry_timeout", 3600);
	calibrationAirmassDistance = getDoubleDefault ("calibration", "airmass_distance", 0.1);
	calibrationLunarDist = getDoubleDefault ("calibration", "lunar_dist", 20);
	calibrationValidTime = getIntegerDefault ("calibration", "valid_time", 3600);
	calibrationMaxDelay = getIntegerDefault ("calibration", "max_delay", 7200);
	getFloat ("calibration", "min_bonus", calibrationMinBonus, 1.0);
	getFloat ("calibration", "max_bonus", calibrationMaxBonus, 300.0);

	getFloat ("swift", "min_horizon", swift_min_horizon, 0);
	getFloat ("swift", "soft_horizon", swift_soft_horizon, swift_min_horizon);

	// GRD section
	grbd_follow_transients = getBoolean ("grbd", "know_transients", true);
	grbd_validity = getIntegerDefault ("grbd", "validity", 86400);

	if (ret)
		return -1;

	return IniParser::getSpecialValues ();
}

Configuration::Configuration (bool defaultSection):IniParser (defaultSection)
{
	observer.lat = 0;
	observer.lng = 0;
	storeSexadecimals = false;
	checker = NULL;
	// default to 120 seconds
	astrometryTimeout = 120;
}

Configuration::~Configuration (void)
{
	delete checker;
}

Configuration * Configuration::instance ()
{
	if (!pInstance)
		pInstance = new Configuration ();
	return pInstance;
}

struct ln_lnlat_posn * Configuration::getObserver ()
{
	return &observer;
}

ObjectCheck * Configuration::getObjectChecker ()
{
	return checker;
}

int Configuration::getDeviceMinFlux (const char *device, double &minFlux)
{
	return getDouble (device, "minflux", minFlux);
}

time_t Configuration::getNight ()
{
	time_t now = getNight (time (NULL));
	struct tm *tm_s = gmtime (&now);
	return getNight (tm_s->tm_year + 1900, tm_s->tm_mon + 1, tm_s->tm_mday);
}

time_t Configuration::getNight (int year, int month, int day)
{
	struct tm _tm;

	if (month > 12)
	{
		year++;
		month -= 12;
	}

	_tm.tm_year = year - 1900;
	_tm.tm_mon = month - 1;
	_tm.tm_mday = day;
	_tm.tm_hour = 12 - getUTOffset ();
	_tm.tm_min = _tm.tm_sec = 0;
#if !(defined (sun) || defined(__CYGWIN__))
	_tm.tm_gmtoff = 0;
	_tm.tm_isdst = 0;
	_tm.tm_zone = "\0";
#endif
	time_t n = mktime (&_tm);
	n -= timezone;

	return n;
}
