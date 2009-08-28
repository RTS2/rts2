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

// maximal movement lenght
#define MAX_MOVE               ((1<<24)-1)
#define MOVE_SLEEP_TIME        5

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

		virtual int commandAuthorized (Rts2Conn * conn);

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

		virtual void addSelectSocks ();
		virtual void selectSuccess ();

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

		// read axis - registers 1-3
		int readAxis (Rts2ConnSerial *conn, Rts2ValueInteger *value, bool write_axis = true);

		void setGuideRa (int value);
		void setGuideDec (int value);
		void setGuidingSpeed (double value);

		void checkAcc (Rts2ConnSerial *conn, Rts2ValueInteger *acc, Rts2ValueInteger *startStop);

		void setSpeedRa (int new_speed);
		void setSpeedDec (int new_speed);

		void setRa (long new_ra);
		void setDec (long new_dec);

		Rts2ValueInteger *raMoving;
		Rts2ValueInteger *decMoving;

		Rts2ValueTime *raMovingEnd;
		Rts2ValueTime *decMovingEnd;

		Rts2ValueBool *wormRa;

		Rts2ValueSelection *raGuide;
		Rts2ValueSelection *decGuide;

		Rts2ValueDouble *guidingSpeed;

		Rts2ValueInteger *unitRa;
		Rts2ValueInteger *unitDec;

		Rts2ValueInteger *velRa;
		Rts2ValueInteger *velDec;

		Rts2ValueInteger *accRa;
		Rts2ValueInteger *accDec;

		Rts2ValueInteger *qRa;
		Rts2ValueInteger *qDec;

		Rts2ValueInteger *microRa;
		Rts2ValueInteger *microDec;

		Rts2ValueInteger *numberRa;
		Rts2ValueInteger *numberDec;

		Rts2ValueInteger *startRa;
		Rts2ValueInteger *startDec;

		Rts2ValueInteger *accWormRa;
		Rts2ValueInteger *velWormRa;
		Rts2ValueInteger *backWormRa;
		Rts2ValueInteger *waitWormRa;

		int32_t ac, dc;

		// mode of RA motor
		enum {MODE_NORMAL, MODE_WORM, MODE_WORM_WAIT} raMode;

		void tel_run (Rts2ConnSerial *conn, int value);
		/**
		 * Stop telescope movement. Phases is bit mask indicating which phase should be commited.
		 * First bit in phase is for sending Kill command, second is for waiting for reply.
		 */
		void tel_kill (Rts2ConnSerial *conn, int phases = 0x03);

		void stopMoveRa ();
		void stopMoveDec ();

		void initMotors ();
		void initRa ();
		void initDec ();
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

int Trencin::startWorm ()
{
	if (raMode == MODE_WORM_WAIT)
	{
		int ret;
		ret = readAxis (trencinConnRa, unitRa, false);
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
		tel_write_ra ("@2\rU1\rU2\rU3\rL10\r");
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
		tel_write_ra ('\\');
		// set proper values for speed after reset
		initRa ();
		raMode = MODE_NORMAL;
	}
	return 0;
}

void Trencin::addSelectSocks ()
{
	if (raMoving->getValueInteger () != 0)
		trencinConnRa->add (&read_set, &write_set, &exp_set);
	if (decMoving->getValueInteger () != 0)
		trencinConnDec->add (&read_set, &write_set, &exp_set);
	Telescope::addSelectSocks ();
}

void Trencin::selectSuccess ()
{
	if (raMoving->getValueInteger () != 0 && trencinConnRa->receivedData (&read_set))
	{
		if (readAxis (trencinConnRa, unitRa, false) == 0)
		{
		 	if (fabs (raMoving->getValueInteger ()) > MAX_MOVE)
			{
				raMoving->setValueInteger (raMoving->getValueInteger () - raMoving->getValueInteger () > 0 ? MAX_MOVE : -MAX_MOVE);
				tel_run (trencinConnRa, raMoving->getValueInteger ());
			}
			else
			{
				raMoving->setValueInteger (0);
			}
			sendValueAll (raMoving);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot read ra " << trencinConnRa->receivedData (&read_set) << sendLog;
		}
	}

	if (decMoving->getValueInteger () != 0 && trencinConnDec->receivedData (&read_set))
	{
		if (readAxis (trencinConnDec, unitDec, false) == 0)
		{
		 	if (fabs (decMoving->getValueInteger ()) > MAX_MOVE)
			{
				decMoving->setValueInteger (decMoving->getValueInteger () - decMoving->getValueInteger () > 0 ? MAX_MOVE : -MAX_MOVE);
				tel_run (trencinConnDec, decMoving->getValueInteger ());
			}
			else
			{
				decMoving->setValueInteger (0);
			}
			sendValueAll (decMoving);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot read dec " << trencinConnDec->receivedData (&read_set) << sendLog;
		}
	}

	Telescope::selectSuccess ();
}

int Trencin::readAxis (Rts2ConnSerial *conn, Rts2ValueInteger *value, bool write_axis)
{
	int ret;
	char buf[10];

	if (write_axis)
	{
		conn->flushPortIO ();
		ret = conn->writePort ("[\rU1\rU2\rU3\r]\r", 13);
		if (ret < 0)
			return -1;
	}
	// read it.
	ret = conn->readPort (buf, 3);
	if (ret < 0)
		return -1;
	value->setValueInteger (((unsigned char) buf[0]) + ((unsigned char) buf[1]) * 256 + ((unsigned char) buf[2]) * 256 * 256);
	return 0;
}

void Trencin::setGuideRa (int value)
{
	stopMoveRa ();
	if (value == 0)
	{
		return;
	}
	switch (value)
	{
		case 1:
			raMoving->setValueInteger (-MAX_MOVE);
			break;
		case 2:
			raMoving->setValueInteger (MAX_MOVE);
			break;
	}
	tel_run (trencinConnRa, raMoving->getValueInteger ());
}

void Trencin::setGuideDec (int value)
{
	stopMoveDec ();
	if (value == 0)
	{
		return;
	}
	switch (value)
	{
		case 1:
			decMoving->setValueInteger (-MAX_MOVE);
			break;
		case 2:
			decMoving->setValueInteger (MAX_MOVE);
			break;
	}
	tel_run (trencinConnDec, decMoving->getValueInteger ());
}

void Trencin::setGuidingSpeed (double value)
{
	if (raGuide->getValueInteger () != 0 || decGuide->getValueInteger () != 0)
	{
		if (raGuide->getValueInteger () != 0)
			tel_kill (trencinConnRa, 0x01);
		if (decGuide->getValueInteger () != 0)
		  	tel_kill (trencinConnDec, 0x01);

		if (raGuide->getValueInteger () != 0)
		  	tel_kill (trencinConnRa, 0x02);
		if (decGuide->getValueInteger () != 0);
			tel_kill (trencinConnDec, 0x02);
	}

	int vel = (value * fabs (haCpd)) / 64;
	setSpeedRa (vel);

	vel = (value * fabs (decCpd)) / 64;
	setSpeedDec (vel);

	sendValueAll (velRa);
	sendValueAll (velDec);

	initRa ();
	initDec ();

	usleep (USEC_SEC / 20);

	if (raGuide->getValueInteger () != 0)
		setGuideRa (raGuide->getValueInteger ());
	if (decGuide->getValueInteger () != 0)
		setGuideDec (decGuide->getValueInteger ());
}

void Trencin::checkAcc (Rts2ConnSerial *conn, Rts2ValueInteger *acc, Rts2ValueInteger *startStop)
{
	if (startStop->getValueInteger () <= 30)
		acc->setValueInteger (116);
	else if (startStop->getValueInteger () <= 61)
		acc->setValueInteger (464);
	else 
		acc->setValueInteger (800);

	tel_write (conn, 'A', acc->getValueInteger ());
	sendValueAll (acc);
}

void Trencin::setSpeedRa (int new_speed)
{
	if (new_speed < 16)
		new_speed = 16;
	tel_write_ra ('[');
	if (new_speed < 200)
	{
		startRa->setValueInteger (new_speed);
	}
	else
	{
		startRa->setValueInteger (200);
	}
	checkAcc (trencinConnRa, accRa, startRa);
	tel_write_ra ('s', startRa->getValueInteger ());
	tel_write_ra ('V', new_speed);
	tel_write_ra (']');
	velRa->setValueInteger (new_speed);
	sendValueAll (startRa);
}

void Trencin::setSpeedDec (int new_speed)
{
	if (new_speed < 16)
		new_speed = 16;
	tel_write_dec ('[');
	if (new_speed < 200)
	{
		startDec->setValueInteger (new_speed);
	}
	else
	{
		startDec->setValueInteger (200);
	}
	checkAcc (trencinConnDec, accDec, startDec);
	tel_write_ra ('s', startDec->getValueInteger ());
	tel_write_dec ('V', new_speed);
	tel_write_ra (']');
	velDec->setValueInteger (new_speed);
	sendValueAll (startDec);
}

void Trencin::setRa (long new_ra)
{
	if (raMoving->getValueInteger () != 0)
	{
		tel_kill (trencinConnRa);
		raMovingEnd->setValueDouble (getNow ());

		readAxis (trencinConnRa, unitRa);
	}
	long diff = new_ra - unitRa->getValueInteger ();
	// adjust for siderial move..
	double v = ((double) velRa->getValueInteger ()) * 64;
	diff *= v / (v + (double) (((diff < 0) ? 1 : -1) * haCpd) / 240.0);
	tel_run (trencinConnRa, diff);
}

void Trencin::setDec (long new_dec)
{
	if (decMoving->getValueInteger () != 0)
	{
		tel_kill (trencinConnDec);
		decMovingEnd->setValueDouble (getNow ());

		readAxis (trencinConnDec, unitDec);
	}

	long diff = new_dec - unitDec->getValueLong ();

	tel_run (trencinConnDec, diff);
}

Trencin::Trencin (int _argc, char **_argv):Fork (_argc, _argv)
{
	trencinConnRa = NULL;
	trencinConnDec = NULL;

	haZero = 0;
	decZero = 0;

	haCpd = -56889;
	decCpd = 110222;

	ra_ticks = (int32_t) (fabs (haCpd) * 360);
	dec_ticks = (int32_t) (fabs (decCpd) * 360);

	acMargin = (int32_t) (haCpd * 5);

	device_nameRa = "/dev/ttyS0";
	device_nameDec = "/dev/ttyS1";
	addOption ('r', NULL, 1, "device file for RA motor (default /dev/ttyS0)");
	addOption ('D', NULL, 1, "device file for DEC motor (default /dev/ttyS1)");

	createValue (raGuide, "ra_guide", "RA guiding status", false);
	raGuide->addSelVal ("NONE");
	raGuide->addSelVal ("MINUS");
	raGuide->addSelVal ("PLUS");

	createValue (decGuide, "dec_guide", "DEC guiding status", false);
	decGuide->addSelVal ("NONE");
	decGuide->addSelVal ("MINUS");
	decGuide->addSelVal ("PLUS");

	createValue (guidingSpeed, "guiding_speed", "guiding speed in deg/sec", false, RTS2_DT_DEGREES);
	guidingSpeed->setValueDouble (0.5);

	createValue (raMoving, "ra_moving", "if RA drive is moving", false);
	raMoving->setValueInteger (0);

	createValue (decMoving, "dec_moving", "if DEC drive is moving", false);
	decMoving->setValueInteger (0);

	createValue (raMovingEnd, "ra_moving_end", "time of end of last RA movement", false);
	createValue (decMovingEnd, "dec_moving_end", "time of end of last DEC movement", false);

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

	createValue (qRa, "qualification_ra", "number of microsteps in top speeds", false);

	createValue (qDec, "qualification_dec", "number of microsteps in top speeds", false);
	
	qRa->setValueInteger (2);
	qDec->setValueInteger (2);

	createValue (microRa, "micro_ra", "RA microstepping", false);
	createValue (microDec, "micro_dec", "DEC microstepping", false);

	microRa->setValueInteger (8);
	microDec->setValueInteger (8);

	createValue (numberRa, "number_ra", "current shape in microstepping", false);
	createValue (numberDec, "number_dec", "current shape in microstepping", false);

	numberRa->setValueInteger (6);
	numberDec->setValueInteger (6);

	createValue (startRa, "start_ra", "start/stop speed", false);
	createValue (startDec, "start_dec", "start/stop speed", false);

	startRa->setValueInteger (200);
	startDec->setValueInteger (200);

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

	try
	{
		initMotors ();
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot init motors" << sendLog;
		return -1;
	}

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
	  	if (old_value == raGuide)
		{
		  	setGuideRa (new_value->getValueInteger ());
			return 0;
		}
		if (old_value == decGuide)
		{
			setGuideDec (new_value->getValueInteger ());
			return 0;
		}
		if (old_value == guidingSpeed)
		{
			setGuidingSpeed (new_value->getValueDouble ());
			return 0;
		}
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
		else if (old_value == qRa)
		{
			tel_write_ra ('q', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == qDec)
		{
			tel_write_dec ('q', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == microRa)
		{
			tel_write_ra ('M', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == microDec)
		{
			tel_write_dec ('M', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == numberRa)
		{
			tel_write_ra ('N', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == numberDec)
		{
			tel_write_dec ('N', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == startRa)
		{
			if (new_value->getValueInteger () > velRa->getValueInteger ())
				return -2;
			tel_write_ra ('s', new_value->getValueInteger ());
			return 0;
		}
		else if (old_value == startDec)
		{
			if (new_value->getValueInteger () > velDec->getValueInteger ())
				return -2;
			tel_write_dec ('s', new_value->getValueInteger ());
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

	int32_t u_ra;
	int32_t u_dec;

	int32_t left_track;

	// update axRa and axDec
	if (raMode == MODE_NORMAL)
	{
		if (raMoving->getValueInteger () == 0)
		{
			readAxis (trencinConnRa, unitRa);
	 		u_ra = unitRa->getValueInteger ();
		}
		else
		{
			left_track = velRa->getValueInteger () * 64 * (raMovingEnd->getValueDouble () - getNow ());
			u_ra = unitRa->getValueInteger () - raMoving->getValueInteger () * (double) left_track / fabs (raMoving->getValueInteger ());
		}
	}

	if (decMoving->getValueInteger () == 0)
	{
		readAxis (trencinConnDec, unitDec);
		u_dec = unitDec->getValueInteger ();
	}
	else
	{
		left_track = velDec->getValueInteger () * 64 * (decMovingEnd->getValueDouble () - getNow ());
		u_dec = unitDec->getValueInteger () - decMoving->getValueInteger () * (double) left_track / fabs (decMoving->getValueInteger ());
	}

	ret = counts2sky (u_ra, u_dec, t_telRa, t_telDec);
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
			if (wormRa->getValueBool () == true)
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

	stopMoveRa ();

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
		tel_kill (trencinConnRa, 0x01);
		tel_kill (trencinConnDec, 0x01);

		raMoving->setValueInteger (0);
		decMoving->setValueInteger (0);

		tel_kill (trencinConnRa, 0x02);
		tel_kill (trencinConnDec, 0x02);

		stopMoveRa ();
		stopMoveDec ();
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

int Trencin::commandAuthorized (Rts2Conn *conn)
{
	if (conn->isCommand ("reset"))
	{
		try
		{
			tel_write_ra ('\\');
			tel_write_dec ('\\');
			initMotors ();
		}
		catch (rts2core::Error &er)
		{
		 	conn->sendCommandEnd (DEVDEM_E_HW, er.what());
			return -1;
		}
		return 0;
	}
	return Fork::commandAuthorized (conn);
}

void Trencin::tel_run (Rts2ConnSerial *conn, int value)
{
	if (value == 0)
		return;
	tel_write (conn, '[');
	if (value > 0)
	{
		if (value > MAX_MOVE)
			tel_write (conn, 'F', MAX_MOVE);
		else
			tel_write (conn, 'F', value);
	}
	else
	{
		if (value < -MAX_MOVE)
			tel_write (conn, 'B', MAX_MOVE);
		else
			tel_write (conn, 'B', -1 * value);
	}
	tel_write (conn, "r\rU1\rU2\rU3\r]\r");
	if (conn == trencinConnRa)
	{
		raMovingEnd->setValueDouble (getNow () + 2 + fabs (value) / (64 * velRa->getValueInteger ()));
		raMoving->setValueInteger (value);

		sendValueAll (raMovingEnd);
		sendValueAll (raMoving);
	}
	else if (conn == trencinConnDec)
	{
	  	decMovingEnd->setValueDouble (getNow () + 2 + fabs (value) / (64 * velDec->getValueInteger ()));
		decMoving->setValueInteger (value);

		sendValueAll (decMovingEnd);
		sendValueAll (decMoving);
	}
}

void Trencin::tel_kill (Rts2ConnSerial *conn, int phases)
{
	if (phases & 0x01)
	{
		tel_write (conn, "K\rU\r");
	}
	if (phases & 0x02)
	{
		char buf;
		int ret;
		conn->setVTime (100);
		ret = conn->readPort (&buf, 1);
		if (ret < 0)
			throw rts2core::Error ("cannot read from port after kill command");
		if (buf != 'U')
			logStream (MESSAGE_ERROR) << "received unexpected character, expected U, got " << buf << sendLog;
		conn->setVTime (40);
		conn->flushPortIO ();
	}
}

void Trencin::stopMoveRa ()
{
	if (raMoving->getValueInteger () != 0)
	{
		tel_kill (trencinConnRa);

		raMoving->setValueInteger (0);
	}

	raMovingEnd->setNow ();

	sendValueAll (raMovingEnd);
	sendValueAll (raMoving);
}

void Trencin::stopMoveDec ()
{
	if (decMoving->getValueInteger () != 0)
	{
		tel_kill (trencinConnDec);

		decMoving->setValueInteger (0);
	}

	decMovingEnd->setNow ();

	sendValueAll (decMovingEnd);
	sendValueAll (decMoving);
}

void Trencin::initMotors ()
{
	initRa ();
	initDec ();
}

void Trencin::initRa ()
{
	tel_write_ra ('M', microRa->getValueInteger ());
	tel_write_ra ('q', qRa->getValueInteger ());
	tel_write_ra ('N', numberRa->getValueInteger ());
	tel_write_ra ('A', accRa->getValueInteger ());
	tel_write_ra ('s', startRa->getValueInteger ());
	tel_write_ra ('V', velRa->getValueInteger ());
}

void Trencin::initDec ()
{
	tel_write_dec ('M', microDec->getValueInteger ());
	tel_write_dec ('q', qDec->getValueInteger ());
	tel_write_dec ('N', numberDec->getValueInteger ());
	tel_write_dec ('A', accDec->getValueInteger ());
	tel_write_dec ('s', startDec->getValueInteger ());
	tel_write_dec ('V', velDec->getValueInteger ());
}

int main (int argc, char **argv)
{
	Trencin device = Trencin (argc, argv);
	return device.run ();
}
