/* 
 * Driver for LX200 GPS telescopes.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

/*!
 * @file Driver for generic TelLX200GPS protocol based telescope.
 *
 * TelLX200GPS was based on original c library for XEphem writen by Ken
 * Shouse <ken@kshouse.engine.swri.edu> and modified by Carlos Guirao
 * <cguirao@eso.org>. But most of the code was completly
 * rewriten and you will barely find any piece from XEphem.
 *
 * @author john,petr,Lee Hicks
 * NOTE: One modification of this version, which is supported by
 * TelLX200GPSGPS only is home positions. The TelLX200GPSGPS when parked must be shutdown
 * and restarted before next observation.
 *
 * What this driver will do is go to home and then sleep,
 * upon next movemnt will wake the telescope up.
 */

#include "rts2lx200/tellx200gps.h"
#include "configuration.h"

#define RATE_FIND 'M'
#define DIR_NORTH 'n'
#define DIR_EAST  'e'
#define DIR_SOUTH 's'
#define DIR_WEST  'w'

using namespace rts2teld;

TelLX200GPS::TelLX200GPS (int in_argc, char **in_argv):TelLX200 (in_argc, in_argv)
{
	motors = 0;
}


TelLX200GPS::~TelLX200GPS (void)
{
}

int TelLX200GPS::initHardware ()
{
	int ret = TelLX200::initHardware ();
	if (ret)
		return ret;
	char rbuf[10];
	// we get 12:34:4# while we're in short mode
	// and 12:34:45 while we're in long mode
	if (serConn->writeRead ("#:Gr#", 5, rbuf, 9, '#') < 0)
		return -1;

	if (rbuf[7] == '\0' || rbuf[7] == '#')
	{
		// that could be used to distinguish between long
		// short mode
		// we are in short mode, set the long on
		if (serConn->writeRead ("#:U#", 5, rbuf, 0) < 0)
			return -1;
		return 0;
	}
	return 0;
}

/*!
 * Reads information about telescope.
 *
 */
int TelLX200GPS::initValues ()
{
	int ret = -1 ;

        rts2core::Configuration *config = rts2core::Configuration::instance ();
        ret = config->loadFile ();
        if (ret)
	  return -1;

        telLongitude->setValueDouble (config->getObserver ()->lng);
        telLatitude->setValueDouble (config->getObserver ()->lat);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());

	if (tel_read_longitude () || tel_read_latitude ())
		return -1;

	strcpy (telType, "TelLX200GPS");
	telAltitude->setValueDouble (600);

	telFlip->setValueInteger (0);

	return Telescope::initValues ();
}

int TelLX200GPS::info ()
{
	if (tel_read_ra () || tel_read_dec ())
		return -1;

	return Telescope::info ();
}

/*!
 * Set slew rate. For completness?
 *
 * This functions are there IMHO mainly for historical reasons. They
 * don't have any use, since if you start move and then die, telescope
 * will move forewer till it doesn't hurt itself. So it's quite
 * dangerous to use them for automatic observation. Better use quide
 * commands from attached CCD, since it defines timeout, which rules
 * CCD.
 *
 * @param new_rate	new rate to set. Uses RATE_<SPEED> constant.
 *
 * @return -1 on failure & set errno, 5 (>=0) otherwise
 */
int TelLX200GPS::tel_set_rate (char new_rate)
{
	char command[6];
	sprintf (command, "#:R%c#", new_rate);
	return serConn->writePort (command, 5);
}

void tel_normalize (double *ra, double *dec)
{
	*ra = ln_range_degrees (*ra);
	if (*dec < -90)
								 //normalize dec
		*dec = floor (*dec / 90) * -90 + *dec;
	if (*dec > 90)
		*dec = *dec - floor (*dec / 90) * 90;
}

/*!
 * Slew (=set) TelLX200GPS to new coordinates.
 *
 * @param ra 		new right ascenation
 * @param dec 		new declination
 *
 * @return -1 on error, otherwise 0
 */
int TelLX200GPS::tel_slew_to (double ra, double dec)
{
	char retstr;

	tel_normalize (&ra, &dec);

	if (tel_write_ra (ra) < 0 || tel_write_dec (dec) < 0)
		return -1;
	if (serConn->writeRead ("#:MS#", 5, &retstr, 1) < 0)
		return -1;
	if (retstr == '0')
		return 0;
	return -1;
}

/*!
 * Check, if telescope match given coordinates.
 *
 * @param ra		target right ascenation
 * @param dec		target declination
 *
 * @return -1 on error, 0 if not matched, 1 if matched, 2 if timeouted
 */
int TelLX200GPS::tel_check_coords (double ra, double dec)
{
	// ADDED BY JF
	double JD;

	double sep;
	time_t now;

	struct ln_equ_posn object, target;
	struct ln_lnlat_posn observer;
	struct ln_hrz_posn hrz;

	time (&now);
	if (now > move_timeout)
		return 2;

	if ((tel_read_ra () < 0) || (tel_read_dec () < 0))
		return -1;

	// ADDED BY JF
	// CALCULATE & PRINT ALT/AZ & HOUR ANGLE TO LOG
	object.ra = getTelRa ();
	object.dec = getTelDec ();

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	JD = ln_get_julian_from_sys ();

	ln_get_hrz_from_equ (&object, &observer, JD, &hrz);

	logStream (MESSAGE_DEBUG) << "TelLX200GPS tel_check_coords TELESCOPE ALT " << hrz.
		alt << " AZ " << hrz.az << sendLog;

	target.ra = ra;
	target.dec = dec;

	sep = ln_get_angular_separation (&object, &target);

	if (sep > 0.1)
		return 0;

	return 1;
}

void TelLX200GPS::set_move_timeout (time_t plus_time)
{
	time_t now;
	time (&now);

	move_timeout = now + plus_time;
}

int TelLX200GPS::startResync ()
{
	if ((getState () & TEL_PARKED) || (getState () & TEL_PARKING))
		serConn->writePort (":PO#", 4);

	tel_slew_to (getTelTargetRa (), getTelTargetDec ());

	set_move_timeout (100);
	return 0;
}

int TelLX200GPS::isMoving ()
{
	char buf[2];
	serConn->writeRead (":D#", 3, buf, 2, '#');
	switch (*buf)
	{
		case '#':
			return -2;
		default:
			return USEC_SEC;
	}
}

int TelLX200GPS::stopMove ()
{
	char dirs[] = { 'e', 'w', 'n', 's' };
	int i;
	for (i = 0; i < 4; i++)
	{
		if (tel_stop_slew_move (dirs[i]) < 0)
			return -1;
	}
	return 0;
}

/*!
 * Set telescope to match given coordinates
 *
 * This function is mainly used to tell the telescope, where it
 * actually is at the beggining of observation (remember, that TelLX200GPS
 * doesn't have absolute position sensors)
 *
 * @param ra		setting right ascennation
 * @param dec		setting declination
 *
 * @return -1 and set errno on error, otherwise 0
 */
int TelLX200GPS::setTo (double ra, double dec)
{
	char readback[101];
	int ret;

	tel_normalize (&ra, &dec);

	if ((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
		return -1;
	if (serConn->writeRead ("#:CM#", 5, readback, 100, '#') < 0)
		return -1;
	// since we are carring operation critical for next movements of telescope,
	// we are obliged to check its correctness
	set_move_timeout (10);
	ret = tel_check_coords (ra, dec);
	return ret == 1;
}


/*!
 * Correct telescope coordinates.
 *
 * Used for closed loop coordinates correction based on astronometry
 * of obtained images.
 *
 * @param ra		ra correction
 * @param dec		dec correction
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int TelLX200GPS::correct (double cor_ra, double cor_dec, double real_ra, double real_dec)
{
	if (setTo (real_ra, real_dec))
		return -1;
	return 0;
}

/*!
 * Park telescope to neutral location.
 *
 * @return -1 and errno on error, 0 otherwise
 */
int TelLX200GPS::startPark ()
{
	int ret = serConn->writePort (":hF#", 4);
	if (ret < 0)
		return -1;
	sleep (1);
	return 0;
}

int TelLX200GPS::isParking ()
{
	char buf[1];
	serConn->writeRead (":h?#", 4, buf, 1);
	switch (*buf)
	{
		case '1':
			return -2;
        case '0':
            return -1;
		default:
			return USEC_SEC;
	}
}

int TelLX200GPS::endPark ()
{
    //possibly sleep telescope
	//int ret = serConn->writePort (":AL#", 4);
	//if (ret < 0)
	//	return -1;
	sleep (1);
	return 0;
}

int TelLX200GPS::startDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
			tel_set_rate (RATE_FIND);
			return tel_start_slew_move (*dir);
	}
	return -2;
}

int TelLX200GPS::stopDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
			return tel_stop_slew_move (*dir);
	}
	return -2;
}
