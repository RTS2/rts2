/* 
 * Driver for Ondrejov, Astrolab Trencin scope.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#define DEBUG_EXTRA

#include "fork.h"

#include "../utils/rts2config.h"
#include "../utils/rts2connserial.h"
#include "../utils/libnova_cpp.h"

namespace rts2teld
{

class Trencin:public Fork
{
	private:
		const char *device_nameRa;
		Rts2ConnSerial *trencinConnRa;

		const char *device_nameDec;
		Rts2ConnSerial *trencinConnDec;

		int tel_write_ra (char command);
		int tel_write_dec (char command);

		// write to both units
		int write_both (char command, int32_t value);

		int tel_write_ra (char command, int32_t value);
		int tel_write_dec (char command, int32_t value);

		Rts2ValueBool *wormRa;

		Rts2ValueInteger *wormRaSpeed;

		Rts2ValueInteger *unitRa;
		Rts2ValueInteger *unitDec;

		Rts2ValueInteger *velRa;
		Rts2ValueInteger *velDec;

		Rts2ValueInteger *accRa;
		Rts2ValueInteger *accDec;

		int32_t ac, dc;

	protected:
		virtual int processOption (int in_opt);

		virtual int getHomeOffset (int32_t & off);

		virtual int isMoving ();
		virtual int isParking ();

		virtual int updateLimits ();

		virtual void updateTrack ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
	public:
		Trencin (int _argc, char **_argv);
		virtual ~ Trencin (void);

		virtual int init ();

		virtual int info ();

		virtual int startResync ();
		virtual int endMove ();
		virtual int stopMove ();

		virtual int startPark ();
		virtual int endPark ();
};

};

using namespace rts2teld;


int
Trencin::tel_write_ra (char command)
{
	char buf[50];
	// switch unit
	int len = snprintf (buf, 2, "%c\r", command);
	return trencinConnRa->writePort (buf, len);
}


int
Trencin::tel_write_dec (char command)
{
	char buf[3];
	// switch unit
	int len = snprintf (buf, 2, "%c\r", command);
	return trencinConnDec->writePort (buf, len);
}


int
Trencin::write_both (char command, int len)
{
	int ret;
	ret = tel_write_ra (command, len);
	if (ret)
		return ret;
	ret = tel_write_dec (command, len);
	return ret;
}


int
Trencin::tel_write_ra (char command, int32_t value)
{
	char buf[51];
	// switch unit
	int len = snprintf (buf, 50, "%c%i\r", command, value);
	return trencinConnRa->writePort (buf, len);
}


int
Trencin::tel_write_dec (char command, int32_t value)
{
	char buf[51];
	// switch unit
	int len = snprintf (buf, 50, "%c%i\r", command, value);
	return trencinConnDec->writePort (buf, len);
}


Trencin::Trencin (int _argc, char **_argv):Fork (_argc, _argv)
{
	trencinConnRa = NULL;
	trencinConnDec = NULL;

	haZero = 0;
	decZero = 0;

	//haCpd = 21333.333;
	//haCpd = 10000;
	haCpd = 10666.666667;
	//decCpd = 17777.778;
	//decCpd = 8100;
	decCpd = 8888.888889;

	ra_ticks = (int32_t) (fabs (haCpd) * 360);
	dec_ticks = (int32_t) (fabs (decCpd) * 360);

	acMargin = (int32_t) (haCpd * 5);

	device_nameRa = "/dev/ttyS0";
	device_nameDec = "/dev/ttyS1";
	addOption ('r', NULL, 1, "device file for RA motor (default /dev/ttyS0");
	addOption ('d', NULL, 1, "device file for DEC motor (default /dev/ttyS1");

	createValue (wormRa, "ra_worm", "RA worm drive", false);
	wormRa->setValueBool (true);

	createValue (wormRaSpeed, "worm_ra_speed", "speed in 25000/x steps per second", false);

	createValue (unitRa, "AXRA", "RA axis raw counts", true);
	unitRa->setValueInteger (0);

	createValue (unitDec, "AXDEC", "DEC axis raw counts", true);
	unitDec->setValueInteger (0);

	createValue (velRa, "vel_ra", "RA velocity", false);
	createValue (velDec, "vel_dec", "DEC velocity", false);

	velRa->setValueInteger (50);
	velDec->setValueInteger (50);

	createValue (accRa, "acc_ra", "RA acceleration", false);
	createValue (accDec, "acc_dec", "DEC acceleration", false);

	accRa->setValueInteger (15);
	accDec->setValueInteger (15);

	// apply all correction for paramount
	setCorrections (true, true, true);
}


Trencin::~Trencin (void)
{
	delete trencinConnRa;
	delete trencinConnDec;
}


int
Trencin::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'r':
			device_nameRa = optarg;
			break;
		case 'd':
			device_nameDec = optarg;
			break;
		default:
			return Fork::processOption (in_opt);
	}
	return 0;
}


int
Trencin::getHomeOffset (int32_t & off)
{
	off = 0;
	return 0;
}


int
Trencin::init ()
{
	int ret;

	ret = Fork::init ();
	if (ret)
		return ret;

	Rts2Config *config = Rts2Config::instance ();
	ret = config->loadFile ();
	if (ret)
		return -1;

	telLongitude->setValueDouble (config->getObserver ()->lng);
	telLatitude->setValueDouble (config->getObserver ()->lat);

	// zero dec is on local meridian, 90 - telLatitude bellow (to nadir)
	decZero = 90 - fabs (telLatitude->getValueDouble ());
	if (telLatitude->getValueDouble () > 0)
		decZero *= -1;
								 // south hemispehere
	if (telLatitude->getValueDouble () < 0)
	{
		// swap values which are opposite for south hemispehere
	}

	trencinConnRa = new Rts2ConnSerial (device_nameRa, this, BS9600, C8, NONE, 40);
	ret = trencinConnRa->init ();
	if (ret)
		return ret;

	trencinConnRa->setDebug ();
	trencinConnRa->flushPortIO ();

	trencinConnDec = new Rts2ConnSerial (device_nameDec, this, BS9600, C8, NONE, 40);
	ret = trencinConnDec->init ();
	if (ret)
		return ret;

	trencinConnDec->setDebug ();
	trencinConnDec->flushPortIO ();

	snprintf (telType, 64, "Trencin");

	wormRaSpeed->setValueInteger (25000);

	return ret;
}


int
Trencin::updateLimits ()
{
	acMin = (int32_t) (haCpd * -180);
	acMax = (int32_t) (haCpd * 180);

	return 0;
}


void
Trencin::updateTrack ()
{
}


int
Trencin::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == unitRa)
	{
		return tel_write_ra ('B',
			new_value->getValueLong ()) == 0 ? 0 : -2;
	}
	if (old_value == unitDec)
	{
		return tel_write_dec ('B',
			new_value->getValueLong ()) == 0 ? 0 : -2;
	}
/*	if (old_value == velRa)
	{
		return tel_write_unit (1, 'v',
			new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == velDec)
	{
		return tel_write_unit (2, 'v',
			new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == accRa)
	{
		return tel_write_unit (1, 'a',
			new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == accDec)
	{
		return tel_write_unit (2, 'a',
			new_value->getValueInteger ()) == 0 ? 0 : -2;
	} */

	return Fork::setValue (old_value, new_value);
}


int
Trencin::info ()
{
	int ret;
	double t_telRa;
	double t_telDec;

	int u_ra = unitRa->getValueInteger ();

	ret = counts2sky (u_ra, unitDec->getValueInteger (), t_telRa, t_telDec);
	setTelRa (t_telRa);
	setTelDec (t_telDec);

	return Fork::info ();
}


int
Trencin::startResync ()
{
	return 0;
}


int
Trencin::isMoving ()
{
	return -2;
}


int
Trencin::endMove ()
{
	return Fork::endMove ();
}


int
Trencin::stopMove ()
{
	int ret;
	ret = tel_write_ra ('K');
	if (ret)
		return ret;
	ret = tel_write_dec ('K');
	if (ret)
		return ret;

	return 0;
}


int
Trencin::startPark ()
{
	return 0;
}


int
Trencin::isParking ()
{	
	int ret;
	ret = info ();
	if (ret)
		return ret;

	if (unitRa->getValueInteger () == 0 && unitDec->getValueInteger () == 0)
		return -2;
	return USEC_SEC / 10;
}


int
Trencin::endPark ()
{
	return 0;
}


int
main (int argc, char **argv)
{
	Trencin device = Trencin (argc, argv);
	return device.run ();
}
