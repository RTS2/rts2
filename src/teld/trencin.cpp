/* 
 * Driver for Astrolab Trencin scope.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "fork.h"

#include "../utils/rts2config.h"
#include "../utils/rts2connserial.h"
#include "../utils/libnova_cpp.h"


/*a ted HODINOVY STROJ:
programova sekvence:
M8
N6
A100
S222
V222
G+
R

takto pojede hodStroj zrhuba tak jak ma...*/

namespace rts2teld
{

class Trencin:public Fork
{
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

	protected:
		virtual int processOption (int in_opt);

		virtual int getHomeOffset (int32_t & off);

		virtual int isMoving ();
		virtual int isParking ();

		virtual int updateLimits ();

		virtual void updateTrack ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		virtual void valueChanged (Rts2Value *changed_value);

	private:
		const char *device_nameRa;
		Rts2ConnSerial *trencinConnRa;

		const char *device_nameDec;
		Rts2ConnSerial *trencinConnDec;

		int tel_write (Rts2ConnSerial *conn, char command);

		int tel_write_ra (char command);
		int tel_write_dec (char command);

		int tel_write (Rts2ConnSerial *conn, const char *command);

		int tel_write_ra (const char *command);
		int tel_write_dec (const char *command);

		// write to both units
		int write_both (char command, int32_t value);

		int tel_write (Rts2ConnSerial *conn, char command, int32_t value);

		int tel_write_ra (char command, int32_t value);
		int tel_write_dec (char command, int32_t value);

		int tel_write_ra_run (char command, int32_t value);
		int tel_write_dec_run (char command, int32_t value);

		// start worm drive on given unit
		int startWorm (Rts2ConnSerial *conn);

		// read axis - registers 1-3
		int readAxis (Rts2ConnSerial *conn, Rts2ValueInteger *value);

		int setRa (long new_ra);
		int setDec (long new_dec);

		Rts2ValueBool *wormRa;

		Rts2ValueInteger *wormRaSpeed;

		Rts2ValueInteger *unitRa;
		Rts2ValueInteger *unitDec;

		Rts2ValueInteger *velRa;
		Rts2ValueInteger *velDec;

		Rts2ValueInteger *accRa;
		Rts2ValueInteger *accDec;

		Rts2ValueInteger *qRa;
		Rts2ValueInteger *microRa;
		Rts2ValueInteger *numberRa;
		Rts2ValueInteger *startRa;

		Rts2ValueInteger *accWormRa;
		Rts2ValueInteger *velWormRa;
		Rts2ValueInteger *backWormRa;
		Rts2ValueInteger *waitWormRa;

		int32_t ac, dc;
};

}

using namespace rts2teld;


int Trencin::tel_write (Rts2ConnSerial *conn, char command)
{
	char buf[3];
	int len = snprintf (buf, 3, "%c\r", command);
	return conn->writePort (buf, len);
}

int Trencin::tel_write_ra (char command)
{
	return tel_write (trencinConnRa, command);
}

int Trencin::tel_write_dec (char command)
{
	return tel_write (trencinConnDec, command);
}

int Trencin::tel_write (Rts2ConnSerial *conn, const char *command)
{
	int ret;
	ret = conn->writePort (command, strlen (command));
	if (ret != 0)
		return ret;
	return tel_write (conn, '\r');	
}

int Trencin::tel_write_ra (const char *command)
{
	return tel_write (trencinConnRa, command);
}


int Trencin::tel_write_dec (const char *command)
{
	return tel_write (trencinConnDec, command);
}




int Trencin::write_both (char command, int len)
{
	int ret;
	ret = tel_write_ra (command, len);
	if (ret)
		return ret;
	ret = tel_write_dec (command, len);
	return ret;
}

int Trencin::tel_write (Rts2ConnSerial *conn, char command, int32_t value)
{
	char buf[51];
	int len = snprintf (buf, 50, "%c%i\r", command, value);
	return conn->writePort (buf, len);
}

int Trencin::tel_write_ra (char command, int32_t value)
{
	return tel_write (trencinConnRa, command, value);
}

int Trencin::tel_write_dec (char command, int32_t value)
{
	return tel_write (trencinConnDec, command, value);
}

int Trencin::tel_write_ra_run (char command, int32_t value)
{
	char buf[51];
	int len = snprintf (buf, 50, "%c%i\rR\r", command, value);
	return trencinConnRa->writePort (buf, len);
}

int Trencin::tel_write_dec_run (char command, int32_t value)
{
	char buf[51];
	int len = snprintf (buf, 50, "%c%i\rR\r", command, value);
	return trencinConnDec->writePort (buf, len);
	return 0;
}

int Trencin::startWorm (Rts2ConnSerial *conn)
{
	int ret;

	ret = tel_write (conn, '[');

	ret = tel_write (conn, 'M', microRa->getValueInteger ());
	if (ret)
		return ret;
	ret = tel_write (conn, 'N', numberRa->getValueInteger ());
	if (ret)
		return ret;
	ret = tel_write (conn, 'A', accWormRa->getValueInteger ());
	if (ret)
		return ret;
	ret = tel_write (conn, 's', startRa->getValueInteger ());
	if (ret)
		return ret;
	ret = tel_write (conn, 'V', velWormRa->getValueInteger ());
	if (ret)
		return ret;
	ret = tel_write (conn, "\r\r@1\r");
	if (ret)
		return ret;
	ret = tel_write (conn, 'B', backWormRa->getValueInteger ());
	if (ret)
		return ret;
	ret = tel_write (conn, "r\rK\r");
	if (ret)
		return ret;
	ret = tel_write (conn, 'W', waitWormRa->getValueInteger ());
	if (ret)
		return ret;
	ret = tel_write (conn, "J1\r]\r");
	if (ret)
		return ret;
	return ret;
}

int Trencin::readAxis (Rts2ConnSerial *conn, Rts2ValueInteger *value)
{
	int ret;
	char buf[10];
	ret = conn->writePort ("U1\r", 3);
	if (ret < 0)
		return -1;
	// read it.
	ret = conn->readPort (buf, 1);
	if (ret < 0)
		return -1;
	ret = conn->writePort ("U2\r", 3);
	if (ret < 0)
		return -1;
	// read it.
	ret = conn->readPort (buf + 1, 1);
	if (ret < 0)
		return -1;
	ret = conn->writePort ("U3\r", 3);
	if (ret < 0)
		return -1;
	// read it.
	ret = conn->readPort (buf + 2, 1);
	if (ret < 0)
		return -1;
	value->setValueInteger (buf[0] + buf[1] * 256 + buf[2] * 256 * 256);
	return 0;
}


int Trencin::setRa (long new_ra)
{
	long diff = unitRa->getValueLong () - new_ra;
	if (diff < 0)
		return tel_write_ra_run ('F', -1 * diff) == 0 ? 0 : -2;
	else if (diff > 0)
	  	return tel_write_ra_run ('B', diff) == 0 ? 0 : -2;
	else return 0;
}

int Trencin::setDec (long new_dec)
{
	long diff = unitDec->getValueLong () - new_dec;
	if (diff < 0)
		return tel_write_dec_run ('F', -1 * diff) == 0 ? 0 : -2;
	else if (diff > 0)
	  	return tel_write_dec_run ('B', diff) == 0 ? 0 : -2;
	else return 0;
}

Trencin::Trencin (int _argc, char **_argv):Fork (_argc, _argv)
{
	trencinConnRa = NULL;
	trencinConnDec = NULL;

	haZero = 0;
	decZero = 0;

	haCpd = 10666.666667;
	decCpd = 8888.888889;

	ra_ticks = (int32_t) (fabs (haCpd) * 360);
	dec_ticks = (int32_t) (fabs (decCpd) * 360);

	acMargin = (int32_t) (haCpd * 5);

	device_nameRa = "/dev/ttyS0";
	device_nameDec = "/dev/ttyS1";
	addOption ('r', NULL, 1, "device file for RA motor (default /dev/ttyS0)");
	addOption ('D', NULL, 1, "device file for DEC motor (default /dev/ttyS1)");

	createValue (wormRa, "ra_worm", "RA worm drive", false);
	wormRa->setValueBool (false);

	createValue (wormRaSpeed, "worm_ra_speed", "speed in 25000/x steps per second", false);

	createValue (unitRa, "AXRA", "RA axis raw counts", true);
	unitRa->setValueInteger (0);

	createValue (unitDec, "AXDEC", "DEC axis raw counts", true);
	unitDec->setValueInteger (0);

	createValue (velRa, "vel_ra", "RA velocity", false);
	createValue (velDec, "vel_dec", "DEC velocity", false);

	velRa->setValueInteger (1500);
	velDec->setValueInteger (1500);

	createValue (accRa, "acc_ra", "RA acceleration", false);
	createValue (accDec, "acc_dec", "DEC acceleration", false);

	accRa->setValueInteger (800);
	accDec->setValueInteger (800);

	createValue (microRa, "micro_ra", "RA microstepping", false);
	microRa->setValueInteger (8);

	createValue (numberRa, "number_ra", "current shape in microstepping", false);
	numberRa->setValueInteger (6);

	createValue (qRa, "qualification_ra", "number of microsteps in top speeds", false);
	qRa->setValueInteger (2);

	createValue (startRa, "start_ra", "start/stop speed");
	startRa->setValueInteger (200);

	createValue (accWormRa, "acc_worm_ra", "acceleration for worm in RA", false);
	accWormRa->setValueInteger (100);

	createValue (velWormRa, "vel_worm_ra", "velocity for worm speeds in RA", false);
	velWormRa->setValueInteger (200);

	createValue (backWormRa, "back_worm_ra", "backward worm trajectory", false);
	backWormRa->setValueInteger (24);

	createValue (waitWormRa, "wait_worm_ra", "wait during RA worm cycle", false);
	waitWormRa->setValueInteger (101);

	// apply all correction for paramount
	setCorrections (true, true, true);
}


Trencin::~Trencin (void)
{
	delete trencinConnRa;
	delete trencinConnDec;
}


int Trencin::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'r':
			device_nameRa = optarg;
			break;
		case 'D':
			device_nameDec = optarg;
			break;
		default:
			return Fork::processOption (in_opt);
	}
	return 0;
}


int Trencin::getHomeOffset (int32_t & off)
{
	off = 0;
	return 0;
}


int Trencin::init ()
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

	trencinConnRa = new Rts2ConnSerial (device_nameRa, this, BS4800, C8, NONE, 40);
	ret = trencinConnRa->init ();
	if (ret)
		return ret;

	trencinConnRa->setDebug ();
	trencinConnRa->flushPortIO ();

	trencinConnDec = new Rts2ConnSerial (device_nameDec, this, BS4800, C8, NONE, 40);
	ret = trencinConnDec->init ();
	if (ret)
		return ret;

	trencinConnDec->setDebug ();
	trencinConnDec->flushPortIO ();

	snprintf (telType, 64, "Trencin");

	tel_write_ra ('M', microRa->getValueInteger ());
	tel_write_ra ('q', qRa->getValueInteger ());
	tel_write_ra ('N', numberRa->getValueInteger ());
	tel_write_ra ('A', accRa->getValueInteger ());
	tel_write_ra ('s', startRa->getValueInteger ());
	tel_write_ra ('V', velRa->getValueInteger ());


	wormRaSpeed->setValueInteger (25000);

	return ret;
}


int Trencin::updateLimits ()
{
	acMin = (int32_t) (haCpd * -180);
	acMax = (int32_t) (haCpd * 180);

	return 0;
}


void Trencin::updateTrack ()
{
}


int Trencin::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == unitRa)
	{
		return setRa (new_value->getValueLong ()) == 0 ? 0 : -2;
	}
	if (old_value == unitDec)
	{
		return setDec (new_value->getValueLong ()) == 0 ? 0 : -2;
	}
	if (old_value == velRa)
	{
		return tel_write_ra ('V', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == velDec)
	{
		return tel_write_dec ('V', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == accRa)
	{
		return tel_write_ra ('A', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == accDec)
	{
		return tel_write_dec ('A', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == microRa)
	{
		return tel_write_ra ('M', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == numberRa)
	{
		return tel_write_ra ('N', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == qRa)
	{
		return tel_write_ra ('q', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == startRa)
	{
		return tel_write_ra ('s', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == wormRa)
	{
		if (((Rts2ValueBool *)new_value)->getValueBool () == true)
			return startWorm (trencinConnRa) == 0 ? 0 : -2;
		return tel_write_ra ('K') == 0 ? 0 : -2;
	}
	if (old_value == accWormRa || old_value == velWormRa
		|| old_value == backWormRa || old_value == waitWormRa)
	{
		return 0;
	}
	return Fork::setValue (old_value, new_value);
}


void Trencin::valueChanged (Rts2Value *changed_value)
{
	if (changed_value == accWormRa || changed_value == velWormRa
		|| changed_value == backWormRa || changed_value == waitWormRa)
	{
		if (wormRa->getValueBool ())
		{
			tel_write_ra ('K');
			startWorm (trencinConnRa);
		}
	}
}


int Trencin::info ()
{
	int ret;
	double t_telRa;
	double t_telDec;

	int u_ra = unitRa->getValueInteger ();

	// update axRa and axDec
	readAxis (trencinConnRa, unitRa);
	readAxis (trencinConnDec, unitDec);

	ret = counts2sky (u_ra, unitDec->getValueInteger (), t_telRa, t_telDec);
	setTelRa (t_telRa);
	setTelDec (t_telDec);

	return Fork::info ();
}


int Trencin::startResync ()
{
	// calculate new X and Y..
	int ret;

	ret = sky2counts (ac, dc);
	if (ret)
		return -1;
	ret = setRa (ac);
	if (ret)
		return -1;
	ret = setDec (dc);
	if (ret)
		return -1;
	return 0;
}


int Trencin::isMoving ()
{
	return -2;
}


int Trencin::endMove ()
{
	startWorm (trencinConnRa);
	return Fork::endMove ();
}


int Trencin::stopMove ()
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


int Trencin::startPark ()
{
	return 0;
}


int Trencin::isParking ()
{	
	int ret;
	ret = info ();
	if (ret)
		return ret;

	if (unitRa->getValueInteger () == 0 && unitDec->getValueInteger () == 0)
		return -2;
	return USEC_SEC / 10;
}


int Trencin::endPark ()
{
	return 0;
}


int main (int argc, char **argv)
{
	Trencin device = Trencin (argc, argv);
	return device.run ();
}
