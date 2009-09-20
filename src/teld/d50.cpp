/* 
 * Driver for Ondrejov, Astrolab D50 scope.
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

class D50:public Fork
{
	private:
		const char *device_name;
		Rts2ConnSerial *d50Conn;

		// write to both units
		int write_both (const char *command, int len);
		int tel_write (const char command, int32_t value);

		int tel_write_unit (int unit, const char command);
		int tel_write_unit (int unit, const char *command, int len);
		int tel_write_unit (int unit, const char command, int32_t value);

		int tel_read (const char command, Rts2ValueInteger * value, Rts2ValueInteger * proc);
		int tel_read_unit (int unit, const char command, Rts2ValueInteger * value, Rts2ValueInteger * proc);

		Rts2ValueBool *motorRa;
		Rts2ValueBool *motorDec;

		Rts2ValueBool *wormRa;
		Rts2ValueBool *wormDec;

		Rts2ValueInteger *wormRaSpeed;

		Rts2ValueBool *moveSleep;

		Rts2ValueInteger *unitRa;
		Rts2ValueInteger *unitDec;

		Rts2ValueInteger *utarRa;
		Rts2ValueInteger *utarDec;

		Rts2ValueInteger *procRa;
		Rts2ValueInteger *procDec;

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
		D50 (int in_argc, char **in_argv);
		virtual ~ D50 (void);

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
D50::write_both (const char *command, int len)
{
	int ret;
	ret = tel_write_unit (1, command, len);
	if (ret)
		return ret;
	ret = tel_write_unit (2, command, len);
	return ret;
}


int
D50::tel_write (const char command, int32_t value)
{
	static char buf[50];
	int len = sprintf (buf, "%c%i\x0d", command, value);
	return d50Conn->writePort (buf, len);
}


int
D50::tel_write_unit (int unit, const char command)
{
	int ret;
	// switch unit
	ret = tel_write ('x', unit);
	if (ret)
		return ret;
	return d50Conn->writePort (command);
}


int
D50::tel_write_unit (int unit, const char *command, int len)
{
	int ret;
	// switch unit
	ret = tel_write ('x', unit);
	if (ret)
		return ret;
	return d50Conn->writePort (command, len);
}


int
D50::tel_write_unit (int unit, const char command, int32_t value)
{
	int ret;
	// switch unit
	ret = tel_write ('x', unit);
	if (ret)
		return ret;
	return tel_write (command, value);
}


int
D50::tel_read (const char command, Rts2ValueInteger * value, Rts2ValueInteger * proc)
{
	int ret;
	static char buf[16];
	ret = d50Conn->writePort (command);
	if (ret)
		return ret;
	// read while there isn't error and we do not get \r
	ret = d50Conn->readPort (buf, 15, '\x0a');
	if (ret == -1) 
	{
		return -1;
	}
	buf[ret] = '\0';

	int vall;
	int ppro;
	ret = sscanf (buf, "#%i&%i\x0d\x0a", &vall, &ppro);
	if (ret != 2)
	{
		ret = sscanf (buf, "#%i", &vall);
		if (ret != 1)
		{
			logStream (MESSAGE_ERROR) << "Wrong buffer " << buf << sendLog;
			d50Conn->flushPortIO ();
			usleep (USEC_SEC);
			return -1;
		}
		ppro = 0;
	}
	value->setValueInteger (vall);
	proc->setValueInteger (ppro);
	return 0;
}


int
D50::tel_read_unit (int unit, const char command, Rts2ValueInteger * value, Rts2ValueInteger * proc)
{
	int ret;
	ret = tel_write ('x', unit);
	if (ret)
		return ret;
	return tel_read (command, value, proc);
}


D50::D50 (int in_argc, char **in_argv)
:Fork (in_argc, in_argv)
{
	d50Conn = NULL;

	haZero = 0;
	decZero = 0;

	//haCpd = 21333.333;
	//haCpd = 10000;
	// zadal Torman 11.5.08 - polovina puvodne vypocitane hodnoty
	haCpd = 10666.666667;
	//decCpd = 17777.778;
	//decCpd = 8100;
	// zadal Torman 11.5.08 - polovina puvodne vypocitane hodnoty
	decCpd = 8888.888889;

	ra_ticks = (int32_t) (fabs (haCpd) * 360);
	dec_ticks = (int32_t) (fabs (decCpd) * 360);

	acMargin = (int32_t) (haCpd * 5);

	device_name = "/dev/ttyS0";
	addOption ('f', "device_name", 1, "device file (default /dev/ttyS0");

	createValue (motorRa, "ra_motor", "power of RA motor", false);
	motorRa->setValueBool (false);
	createValue (motorDec, "dec_motor", "power of DEC motor", false);
	motorDec->setValueBool (false);

	createValue (wormRa, "ra_worm", "RA worm drive", false);
	wormRa->setValueBool (true);
	createValue (wormDec, "dec_worm", "DEC worm drive", false);
	wormDec->setValueBool (false);

	createValue (wormRaSpeed, "worm_ra_speed", "speed in 25000/x steps per second", false);

	createValue (moveSleep, "move_sleep", "sleep this number of seconds after finishing", false);
	moveSleep->setValueInteger (7);

	createValue (unitRa, "AXRA", "RA axis raw counts", true);
	createValue (unitDec, "AXDEC", "DEC axis raw counts", true);

	createValue (utarRa, "tar_axra", "Target RA axis value", true);
	createValue (utarDec, "tar_adec", "Target DEC axis value", true);

	createValue (procRa, "proc_ra", "state for RA processor", false, RTS2_DT_HEX);
	createValue (procDec, "proc_dec", "state for DEC processor", false, RTS2_DT_HEX);

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


D50::~D50 (void)
{
	delete d50Conn;
}


int
D50::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_name = optarg;
			break;
		default:
			return Fork::processOption (in_opt);
	}
	return 0;
}


int
D50::getHomeOffset (int32_t & off)
{
	off = 0;
	return 0;
}


int
D50::init ()
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

	d50Conn = new Rts2ConnSerial (device_name, this, BS9600, C8, NONE, 40);
	ret = d50Conn->init ();
	if (ret)
		return ret;

	d50Conn->setDebug ();
	d50Conn->flushPortIO ();


	snprintf (telType, 64, "D50");

	// switch both motors off
	ret = write_both ("D\x0d", 2);
	if (ret)
		return ret;
	motorRa->setValueBool (false);
	motorDec->setValueBool (false);

	ret = tel_write_unit (1, 'h', 25000);
	if (ret)
		return ret;
	wormRaSpeed->setValueInteger (25000);

	return ret;
}


int
D50::updateLimits ()
{
	acMin = (int32_t) (haCpd * -180);
	acMax = (int32_t) (haCpd * 180);

	return 0;
}


void
D50::updateTrack ()
{
}


int
D50::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == motorRa)
	{
		return tel_write_unit (1,
			((Rts2ValueBool *) new_value)->getValueBool ()? "E\x0d" : "D\x0d", 2) == 0 ? 0 : -2;
	}
	if (old_value == motorDec)
	{
		return tel_write_unit (2,
			((Rts2ValueBool *) new_value)->getValueBool ()? "E\x0d" : "D\x0d", 2) == 0 ? 0 : -2;
	}
	if (old_value == wormRa)
	{
		return tel_write_unit (1,
			((Rts2ValueBool *) new_value)->getValueBool ()? "o0" : "c0", 2) == 0 ? 0 : -2;

	}
	if (old_value == wormDec)
	{
		return tel_write_unit (2,
			((Rts2ValueBool *) new_value)->getValueBool ()? "o0" : "c0", 2) == 0 ? 0 : -2;

	}
	if (old_value == wormRaSpeed)
	{
		return tel_write_unit (1, 'h',
			new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == unitRa)
	{
		return tel_write_unit (1, 's',
			new_value->getValueLong ()) == 0 ? 0 : -2;
	}
	if (old_value == unitDec)
	{
		return tel_write_unit (2, 's',
			new_value->getValueLong ()) == 0 ? 0 : -2;
	}
	if (old_value == velRa)
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
	}

	return Fork::setValue (old_value, new_value);
}


int
D50::info ()
{
	int ret;
	ret = tel_read_unit (1, 'u', unitRa, procRa);
	if (ret)
		return ret;

	ret = tel_read_unit (2, 'u', unitDec, procDec);
	if (ret)
		return ret;

	double t_telRa;
	double t_telDec;

	int u_ra = unitRa->getValueInteger ();

	ret = counts2sky (u_ra, unitDec->getValueInteger (), t_telRa, t_telDec);
	setTelRa (t_telRa);
	setTelDec (t_telDec);

	return Fork::info ();
}


int
D50::startResync ()
{
	int ret;

	// writes again speed and acceleration info
	ret = tel_write_unit (1, 'v', velRa->getValueInteger ());
	if (ret)
		return ret;

	ret = tel_write_unit (2, 'v', velDec->getValueInteger ());
	if (ret)
		return ret;

	ret = tel_write_unit (1, 'a', accRa->getValueInteger ());
	if (ret)
		return ret;

	ret = tel_write_unit (2, 'a', accDec->getValueInteger ());
	if (ret)
		return ret;

	// turn on RA worm
	ret = tel_write_unit (1, "o0", 2);
	if (ret)
		return ret;
	wormRa->setValueBool (true);

	usleep (USEC_SEC / 5);

	ret = sky2counts (ac, dc);
	if (ret)
		return -1;

	utarRa->setValueInteger (ac);
	utarDec->setValueInteger (dc);

	sendValueAll (utarRa);
	sendValueAll (utarDec);

	ret = tel_write_unit (1, 't', ac);
	if (ret)
		return ret;

	usleep (USEC_SEC / 5);

	ret = d50Conn->writePort ('g');
	if (ret)
		return ret;

	ret = tel_write_unit (2, 't', dc);
	if (ret)
		return ret;

	usleep (USEC_SEC / 5);

	ret = d50Conn->writePort ('g');
	if (ret)
		return -1;
	
	return 0;
}


int
D50::isMoving ()
{
	int ret;
	ret = info ();
	if (ret)
		return ret;
	// compute distance - 6 arcmin is expected current limit
	if (getTargetDistance () > 2)
		return USEC_SEC / 10;
	// wait to move to dest
	sleep (moveSleep->getValueInteger ());
	// we reached destination
	return -2;
}


int
D50::endMove ()
{
	return Fork::endMove ();
}


int
D50::stopMove ()
{
	int ret;
	ret = tel_write_unit (1, 'k');
	if (ret)
		return ret;
	ret = tel_write_unit (2, 'k');
	if (ret)
		return ret;

	return 0;
}


int
D50::startPark ()
{
	int ret;
	// switch off worms..
	ret = tel_write_unit (1, "c0", 2);
	if (ret)
		return ret;
	wormRa->setValueBool (false);
	ret = tel_write_unit (2, "c0", 2);
	if (ret)
		return ret;
	wormDec->setValueBool (false);
	ret = tel_write_unit (1, 't', 0);
	if (ret)
		return ret;
	ret = d50Conn->writePort ('g');
	if (ret)
		return -1;
	ret = tel_write_unit (2, 't', 0);
	if (ret)
		return ret;
	ret = d50Conn->writePort ('g');
	if (ret)
		return -1;
	return 0;
}


int
D50::isParking ()
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
D50::endPark ()
{
	return 0;
}


int
main (int argc, char **argv)
{
	D50 device = D50 (argc, argv);
	return device.run ();
}
