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

#include "rts2config.h"

Rts2Config *Rts2Config::pInstance = NULL;

int
Rts2Config::getSpecialValues ()
{
	int ret = 0;
	std::string horizon_file;

	// get some commonly used values
	ret += getDouble ("observatory", "longtitude", observer.lng);
	ret += getDouble ("observatory", "latitude", observer.lat);
	// load horizont file..
	getString ("observatory", "horizon", horizon_file, "");

	obs_epoch_id = 1;
	getInteger ("observatory", "epoch_id", obs_epoch_id);

	getStringVector ("observatory", "required_devices", obs_requiredDevices);

	getString ("observatory", "que_path", obs_quePath, "%b/que/%c/%f");
	getString ("observatory", "acq_path", obs_acqPath, "%b/acqusition/%t/%c/%f");
	getString ("observatory", "archive_path", obs_archive, "%b/archive/%t/%c/object/%f");
	getString ("observatory", "trash_path", obs_trash, "%b/trash/%t/%c/%f");
	getString ("observatory", "flat_path", obs_flats, "%b/flat/%c/raw/%f");
	getString ("observatory", "dark_path", obs_darks, "%b/darks/%c/%f");

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
