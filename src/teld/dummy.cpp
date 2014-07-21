/* 
 * Dummy telescope for tests.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#define OPT_MOVE_FAST    OPT_LOCAL + 510

/*!
 * Dummy teld for testing purposes.
 */

namespace rts2teld
{

/**
 * Dummy telescope class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Markus Wildi
 */
class Dummy:public Telescope
{
	public:
	        Dummy (int argc, char **argv);

	protected:
        	virtual int processOption (int in_opt);
		virtual int initValues ()
		{
			rts2core::Configuration *config;
			config = rts2core::Configuration::instance ();
			config->loadFile ();
			telLatitude->setValueDouble (config->getObserver ()->lat);
			telLongitude->setValueDouble (config->getObserver ()->lng);
			telAltitude->setValueDouble (config->getObservatoryAltitude ());
			strcpy (telType, "Dummy");
			return Telescope::initValues ();
		}

		virtual int info ()
		{
			setTelRaDec (dummyPos.ra, dummyPos.dec);
			julian_day->setValueDouble (ln_get_julian_from_sys ());
			return Telescope::info ();
		}

		virtual int startResync ();

		virtual int startMoveFixed (double tar_az, double tar_alt)
		{
			return 0;
		}

		virtual int stopMove ()
		{
			return 0;
		}

		virtual int startPark ()
		{
			dummyPos.ra = 2;
			dummyPos.dec = 2;
			return 0;
		}

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

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

	private:

                struct ln_equ_posn dummyPos;
                rts2core::ValueBool *move_fast;

		rts2core::ValueDouble *crpix_aux1;
		rts2core::ValueDouble *crpix_aux2;

		rts2core::ValueDouble *julian_day;
};

}

using namespace rts2teld;

Dummy::Dummy (int argc, char **argv):Telescope (argc,argv)
{
	createValue (move_fast, "MOVE_FAST", "fast: reach target position fast, else: slow", false, RTS2_VALUE_WRITABLE);
	move_fast->setValueBool (false);

	createValue (crpix_aux1, "CRPIX1AG", "autoguider offset", false, RTS2_DT_AUXWCS_CRPIX1 | RTS2_VALUE_WRITABLE);
	createValue (crpix_aux2, "CRPIX2AG", "autoguider offset", false, RTS2_DT_AUXWCS_CRPIX2 | RTS2_VALUE_WRITABLE);

	createValue (julian_day, "JULIAN_DAY", "Julian date", false);

	crpix_aux1->setValueDouble (10);
	crpix_aux2->setValueDouble (20);

	dummyPos.ra = 0;
	dummyPos.dec = 0;

	addOption (OPT_MOVE_FAST, "move", 1, "fast: reach target position fast, else: slow (default: 2 deg/sec)");
}

int Dummy::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_MOVE_FAST:
			if (!strcmp( "fast", optarg)) move_fast->setValueBool ( true );
				else move_fast->setValueBool ( false );
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int Dummy::startResync ()
{
	// check if target is above/below horizon
	struct ln_equ_posn tar;
	struct ln_hrz_posn hrz;
	struct ln_equ_posn model_change;

	double JD = ln_get_julian_from_sys ();

	getTarget (&tar);
	applyModel (&tar, &model_change, telFlip->getValueInteger (), JD);

	setTarget (tar.ra, tar.dec);

	ln_get_hrz_from_equ (&tar, rts2core::Configuration::instance ()->getObserver (), JD, &hrz);
	if (hrz.alt < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot move to negative altitude (" << hrz.alt << ")" << sendLog;
		return -1;
	}
	return 0;
}

int Dummy::isMoving ()
{
	if (move_fast->getValueBool () || getNow () > getTargetReached ())
	{
 		struct ln_equ_posn tar;
		getTelTargetRaDec (&tar);
		dummyPos.ra = tar.ra ;
		dummyPos.dec = tar.dec ;
		return -2;
	}
	struct ln_equ_posn tar;
	getTelTargetRaDec (&tar);
	if (fabs (dummyPos.ra - tar.ra) < 0.5)
		dummyPos.ra = tar.ra;
	else if (dummyPos.ra > tar.ra)
		dummyPos.ra -= 0.5;
	else
		dummyPos.ra += 0.5;

	if (fabs (dummyPos.dec - tar.dec) < 0.5)
		dummyPos.dec = tar.dec;
	else if (dummyPos.dec > tar.dec)
		dummyPos.dec -= 0.5;
	else
		dummyPos.dec += 0.5;
	setTelRaDec (dummyPos.ra, dummyPos.dec);
	// position reached
	if (dummyPos.ra == tar.ra && dummyPos.dec == tar.dec)
		return -2;
	return USEC_SEC;
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
