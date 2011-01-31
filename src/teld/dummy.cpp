/* 
 * Dummy telescope for tests.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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
#include "../utils/rts2config.h"

#define OPT_MOVE_FAST    OPT_LOCAL + 510

/*!
 * Dummy teld for testing purposes.
 */

namespace rts2teld
{

class Dummy:public Telescope
{
	private:

                struct ln_equ_posn dummyPos;
                rts2core::ValueBool *move_fast;
	public:
	        Dummy (int argc, char **argv);
        	virtual int processOption (int in_opt);
		virtual int startResync ()
		{
			return 0;
		}

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

	protected:
		virtual int initValues ()
		{
			Rts2Config *config;
			config = Rts2Config::instance ();
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
			return Telescope::info ();
		}

		virtual int isMoving ()
		{
		    if (move_fast->getValueBool ()) {
 			struct ln_equ_posn tar;
			getTelTargetRaDec (&tar);
			dummyPos.ra= tar.ra ;
			dummyPos.dec= tar.dec ;
			return -2;

		    } else {
			if (getNow () > getTargetReached ())
				return -2;
			struct ln_equ_posn tar;
			getTelTargetRaDec (&tar);
			if (dummyPos.ra > tar.ra)
				dummyPos.ra -= 0.5;
			else
				dummyPos.ra += 0.5;
			if (dummyPos.dec > tar.dec)
				dummyPos.dec -= 0.5;
			else
				dummyPos.dec += 0.5;
			setTelRaDec (dummyPos.ra, dummyPos.dec);
			return USEC_SEC;
		    }
		}

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

};

}

using namespace rts2teld;

Dummy::Dummy (int argc, char **argv):Telescope (argc,argv)
{
	addOption (OPT_MOVE_FAST, "move", 1, "fast: reach target position fast, else: slow (default: 2 deg/sec)");
	createValue (move_fast, "MOVE_FAST", "fast: reach target position fast, else: slow", false, RTS2_VALUE_WRITABLE);
	move_fast->setValueBool (false);

	dummyPos.ra = 0;
	dummyPos.dec = 0;
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

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
