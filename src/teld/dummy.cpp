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

#define OPT_MOVE_FAST      OPT_LOCAL + 510

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
			setTelLongLat (config->getObserver ()->lng, config->getObserver ()->lat);
			setTelAltitude (config->getObservatoryAltitude ());
			trackingInterval->setValueFloat (0.5);
			return Telescope::initValues ();
		}

		virtual int info ()
		{
			setTelRaDec (dummyPos.ra, dummyPos.dec);
			julian_day->setValueDouble (ln_get_julian_from_sys ());
			return Telescope::info ();
		}

		virtual int startResync ();

		virtual int endMove ()
		{
			startTracking ();
			return Telescope::endMove ();
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

		virtual void runTracking ();

		virtual int sky2counts (const double utc1, const double utc2, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, bool writeValues);

	private:

                struct ln_equ_posn dummyPos;
                rts2core::ValueBool *move_fast;

		rts2core::ValueDouble *crpix_aux1;
		rts2core::ValueDouble *crpix_aux2;

		rts2core::ValueDouble *julian_day;

		rts2core::ValueDouble *recalDelay;

		rts2core::ValueLong *t_axRa;
		rts2core::ValueLong *t_axDec;
};

}

using namespace rts2teld;

Dummy::Dummy (int argc, char **argv):Telescope (argc, argv, true, true)
{
	createValue (move_fast, "MOVE_FAST", "fast: reach target position fast, else: slow", false, RTS2_VALUE_WRITABLE);
	move_fast->setValueBool (false);

	createValue (crpix_aux1, "CRPIX1AG", "autoguider offset", false, RTS2_DT_AUXWCS_CRPIX1 | RTS2_VALUE_WRITABLE);
	createValue (crpix_aux2, "CRPIX2AG", "autoguider offset", false, RTS2_DT_AUXWCS_CRPIX2 | RTS2_VALUE_WRITABLE);

	crpix_aux1->setValueDouble (10);
	crpix_aux2->setValueDouble (20);

	createValue (julian_day, "JULIAN_DAY", "Julian date", false);

	createValue (recalDelay, "recal_delay", "recalculate position every n second", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE);
	createValue (t_axRa, "T_AXRA", "RA axis target position", false);
	createValue (t_axDec, "T_AXDEC", "DEC axis target position", false);

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
	struct ln_equ_posn tar, tt_pos;
	struct ln_hrz_posn hrz;
	struct ln_equ_posn model_change;

	double JD = ln_get_julian_from_sys ();

	getTarget (&tar);
        getTarget (&tt_pos);
	applyModel (&tar, &tt_pos, &model_change, JD);

	setTarget (tar.ra, tar.dec);
	setTelTarget (tar.ra, tar.dec);

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

void Dummy::runTracking ()
{
	double JD = ln_get_julian_from_sys () + 2 / 86400.0;
	struct ln_equ_posn target;
	int32_t t_ac, t_dc;

	t_ac = t_axRa->getValueLong ();
	t_dc = t_axDec->getValueLong ();

	int ret = calculateTarget (JD + 2 / 86400.0, &target, t_ac, t_dc, true, 0, false);
	if (ret)
	{
		setTracking (0);
		return;
	}

        setTelTarget (target.ra, target.dec);

	t_axRa->setValueLong (target.ra * 10000);
	t_axDec->setValueLong (target.dec * 10000);

	Telescope::runTracking ();
}

int Dummy::sky2counts (const double utc1, const double utc2, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, bool writeValues)
{
	ac = pos->ra * 10000;
	dc = pos->dec * 10000;
	applyCorrections (pos, utc1, utc2, writeValues);
	if (writeValues)
		setTelTarget (pos->ra, pos->dec);
	return 0;
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
