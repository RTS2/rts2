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

#include "../utils/error.h"
#include "../utils/rts2config.h"
#include "../utils/rts2connserial.h"
#include "../utils/libnova_cpp.h"

#define EVENT_TIMER_RA_WORM    RTS2_LOCAL_EVENT + 1230

namespace rts2teld
{

class Trencin:public Fork
{
	public:
		Trencin (int _argc, char **_argv);
		virtual ~ Trencin (void);

		virtual int init ();

		virtual int info ();

		virtual void postEvent (Rts2Event *event);

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

		// start worm drive on given unit
		virtual int startWorm ();
		virtual int stopWorm ();

	private:
		const char *device_nameRa;
		Rts2ConnSerial *trencinConnRa;

		const char *device_nameDec;
		Rts2ConnSerial *trencinConnDec;

		void tel_write (Rts2ConnSerial *conn, char command);

		void tel_write_ra (char command);
		void tel_write_dec (char command);

		void tel_write (Rts2ConnSerial *conn, const char *command);

		void tel_write_ra (const char *command);
		void tel_write_dec (const char *command);

		// write to both units
		void write_both (char command, int32_t value);

		void tel_write (Rts2ConnSerial *conn, char command, int32_t value);

		void tel_write_ra (char command, int32_t value);
		void tel_write_dec (char command, int32_t value);

		void tel_write_ra_run (char command, int32_t value);
		void tel_write_dec_run (char command, int32_t value);

		// read axis - registers 1-3
		int readAxis (Rts2ConnSerial *conn, Rts2ValueInteger *value);

		void setRa (long new_ra);
		void setDec (long new_dec);

		Rts2ValueBool *wormRa;

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

		// mode of RA motor
		enum {MODE_NORMAL, MODE_WORM, MODE_WORM_WAIT} raMode;
};

}

using namespace rts2teld;


void Trencin::tel_write (Rts2ConnSerial *conn, char command)
{
	char buf[3];
	int len = snprintf (buf, 3, "%c\r", command);
	if (conn->writePort (buf, len))
		throw rts2core::Error ("cannot write to port");
}

void Trencin::tel_write_ra (char command)
{
	tel_write (trencinConnRa, command);
}

void Trencin::tel_write_dec (char command)
{
	tel_write (trencinConnDec, command);
}

void Trencin::tel_write (Rts2ConnSerial *conn, const char *command)
{
	if (conn->writePort (command, strlen (command)))
		throw rts2core::Error ("cannot write to port");
	tel_write (conn, '\r');	
}

void Trencin::tel_write_ra (const char *command)
{
	tel_write (trencinConnRa, command);
}


void Trencin::tel_write_dec (const char *command)
{
	tel_write (trencinConnDec, command);
}


void Trencin::write_both (char command, int len)
{
	tel_write_ra (command, len);
	tel_write_dec (command, len);
}

void Trencin::tel_write (Rts2ConnSerial *conn, char command, int32_t value)
{
	char buf[51];
	int len = snprintf (buf, 50, "%c%i\r", command, value);
	if (conn->writePort (buf, len))
		throw rts2core::Error ("cannot write to port");
}

void Trencin::tel_write_ra (char command, int32_t value)
{
	tel_write (trencinConnRa, command, value);
}

void Trencin::tel_write_dec (char command, int32_t value)
{
	tel_write (trencinConnDec, command, value);
}

void Trencin::tel_write_ra_run (char command, int32_t value)
{
	char buf[51];
	int len = snprintf (buf, 50, "%c%i\rR\r", command, value);
	if (trencinConnRa->writePort (buf, len))
		throw rts2core::Error ("cannot write to RA port");
}

void Trencin::tel_write_dec_run (char command, int32_t value)
{
	char buf[51];
	int len = snprintf (buf, 50, "%c%i\rR\r", command, value);
	if (trencinConnDec->writePort (buf, len))
	  	throw rts2core::Error ("cannot write to DEC port");
}

int Trencin::startWorm ()
{
	if (raMode == MODE_WORM_WAIT)
	{
		int ret;
		char buf[2];
		ret = trencinConnRa->readPort (buf, 1);
		raMode = MODE_WORM;
		if (ret < 0)
			return -1;
	}
	if (raMode == MODE_NORMAL)
	{
		tel_write_ra ('[');
		tel_write_ra ('M', microRa->getValueInteger ());
		tel_write_ra ('N', numberRa->getValueInteger ());
		tel_write_ra ('A', accWormRa->getValueInteger ());
		tel_write_ra ('s', startRa->getValueInteger ());
		tel_write_ra ('V', velWormRa->getValueInteger ());
		tel_write_ra ("@2\rU1\rL10\r");
		tel_write_ra ('B', backWormRa->getValueInteger ());
		tel_write_ra ("r\rK\r");
		tel_write_ra ('W', waitWormRa->getValueInteger ());
		tel_write_ra ("E\rJ2\r]\r");
		raMode = MODE_WORM;
	}

	raMode = MODE_WORM_WAIT;
	addTimer (0.75, new Rts2Event (EVENT_TIMER_RA_WORM, this));
	return 0;
}

int Trencin::stopWorm ()
{
	if (raMode != MODE_NORMAL)
	{
		tel_write_ra ('K');
		raMode = MODE_NORMAL;
	}
	return 0;
}

int Trencin::readAxis (Rts2ConnSerial *conn, Rts2ValueInteger *value)
{
	int ret;
	char buf[10];

	// wait for U1 from WORM, if it's needed..
	if (conn == trencinConnRa && raMode == MODE_WORM_WAIT)
	{
		ret = conn->readPort (buf, 1);
		raMode = MODE_WORM;
		if (ret < 0)
			return -1;
	}

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
	value->setValueInteger (((unsigned char) buf[0]) + ((unsigned char) buf[1]) * 256 + ((unsigned char) buf[2]) * 256 * 256);
	return 0;
}


void Trencin::setRa (long new_ra)
{
	long diff = unitRa->getValueLong () - new_ra;
	if (diff < 0)
		tel_write_ra_run ('F', -1 * diff);
	else if (diff > 0)
	  	tel_write_ra_run ('B', diff);
}

void Trencin::setDec (long new_dec)
{
	long diff = unitDec->getValueLong () - new_dec;
	if (diff < 0)
		tel_write_dec_run ('F', -1 * diff);
	else if (diff > 0)
	  	tel_write_dec_run ('B', diff);
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

	// apply all corrections
	setCorrections (true, true, true);

	raMode = MODE_NORMAL;
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

	tel_write_ra ('\\');
	tel_write_ra ('M', microRa->getValueInteger ());
	tel_write_ra ('q', qRa->getValueInteger ());
	tel_write_ra ('N', numberRa->getValueInteger ());
	tel_write_ra ('A', accRa->getValueInteger ());
	tel_write_ra ('s', startRa->getValueInteger ());
	tel_write_ra ('V', velRa->getValueInteger ());

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
	try
	{
		if (old_value == unitRa)
		{
			setRa (new_value->getValueLong ());
			return 0;
		}
		else if (old_value == unitDec)
		{
			setDec (new_value->getValueLong ());
			return 0;
		}
		else if (old_value == velRa)
		{
			tel_write_ra ('V', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == velDec)
		{
			tel_write_dec ('V', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == accRa)
		{
			tel_write_ra ('A', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == accDec)
		{
			tel_write_dec ('A', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == microRa)
		{
			tel_write_ra ('M', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == numberRa)
		{
			tel_write_ra ('N', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == qRa)
		{
			tel_write_ra ('q', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == startRa)
		{
			tel_write_ra ('s', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == wormRa)
		{
			if (((Rts2ValueBool *)new_value)->getValueBool () == true)
				startWorm ();
			else
				stopWorm ();
			return 0;
		}
		else if (old_value == accWormRa || old_value == velWormRa
			|| old_value == backWormRa || old_value == waitWormRa)
		{
			return 0;
		}
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -2;
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
			try
			{
				stopWorm ();
				startWorm ();
			}
			catch (rts2core::Error &er)
			{
				logStream (MESSAGE_ERROR) << er << sendLog;
			}
		}
	}
	Fork::valueChanged (changed_value);
}


int Trencin::info ()
{
	int ret;
	double t_telRa;
	double t_telDec;

	// update axRa and axDec
	readAxis (trencinConnRa, unitRa);
	readAxis (trencinConnDec, unitDec);

	int32_t u_ra = unitRa->getValueInteger ();

	ret = counts2sky (u_ra, unitDec->getValueInteger (), t_telRa, t_telDec);
	setTelRa (t_telRa);
	setTelDec (t_telDec);

	return Fork::info ();
}


void Trencin::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_TIMER_RA_WORM:
			// restart worm..
			startWorm ();
			// do not process it through full hierarchy..
			Rts2Object::postEvent (event);
			return;
	}
	Fork::postEvent (event);
}


int Trencin::startResync ()
{
	// calculate new X and Y..
	int ret;

	ret = sky2counts (ac, dc);
	if (ret)
		return -1;
	try
	{
		setRa (ac);
		setDec (dc);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}


int Trencin::isMoving ()
{
	return -2;
}


int Trencin::endMove ()
{
	startWorm ();
	return Fork::endMove ();
}


int Trencin::stopMove ()
{
	try 
	{
		tel_write_ra ('K');
		tel_write_dec ('K');
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "stopMove: " << er << sendLog;
		return -1;
	}
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
