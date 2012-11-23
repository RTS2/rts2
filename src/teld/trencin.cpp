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

#include "error.h"
#include "configuration.h"
#include "libnova_cpp.h"

#include "connection/serial.h"

#define DEBUG_MOVE  1

#define EVENT_TIMER_RA_WORM    RTS2_LOCAL_EVENT + 1230

#define DEBUG_MOVE  1

// maximal movement lenght
#define MAX_MOVE               ((1<<24)-1)
#define MOVE_SLEEP_TIME        5

#define OPT_PARK_POS           OPT_LOCAL + 571
#define OPT_ASSSUME_0_PARK     OPT_LOCAL + 572

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

		virtual void startOffseting (rts2core::Value *changed_value);

		virtual int startPark ();
		virtual int endPark ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int in_opt);

		virtual int getHomeOffset (int32_t & off);

		virtual int setTo (double set_ra, double set_dec);

		virtual int isMoving ();
		virtual int isParking ();

		virtual int updateLimits ();

		virtual void updateTrack ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void valueChanged (rts2core::Value *changed_value);

		// start worm drive on given unit
		virtual int startWorm ();
		virtual int stopWorm ();

		virtual void addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set);
		virtual void selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set);

	private:
		const char *device_nameRa;
		rts2core::ConnSerial *trencinConnRa;

		const char *device_nameDec;
		rts2core::ConnSerial *trencinConnDec;

		rts2core::ValueAltAz *parkPos;
		bool assumeParked;

		void tel_write (rts2core::ConnSerial *conn, char command);

		void tel_write_ra (char command);
		void tel_write_dec (char command);

		void tel_write (rts2core::ConnSerial *conn, const char *command);

		void tel_write_ra (const char *command);
		void tel_write_dec (const char *command);

		// write to both units
		void write_both (char command, int32_t value);

		void tel_write (rts2core::ConnSerial *conn, char command, int32_t value);

		void tel_write_ra (char command, int32_t value);
		void tel_write_dec (char command, int32_t value);

		// read axis - registers 1-3
		int readAxis (rts2core::ConnSerial *conn, rts2core::ValueInteger *value, bool write_axis = true);

		void setGuideRa (int value);
		void setGuideDec (int value);
		void setGuidingSpeed (double value);

		void checkAcc (rts2core::ConnSerial *conn, rts2core::ValueInteger *acc, rts2core::ValueInteger *startStop);

		void setSpeedRa (int new_speed);
		void setSpeedDec (int new_speed);

		void setRa (long new_ra);
		void setDec (long new_dec);

		rts2core::ValueInteger *raMoving;
		rts2core::ValueInteger *decMoving;

		rts2core::ValueTime *raMovingEnd;
		rts2core::ValueTime *decMovingEnd;

		rts2core::ValueBool *wormRa;
		rts2core::ValueTime *raWormStart;

		int32_t worm_start_unit_ra;

		rts2core::ValueDouble *guidingSpeed;

		rts2core::ValueInteger *unitRa;
		rts2core::ValueInteger *unitDec;

		rts2core::ValueInteger *cycleRa;
		rts2core::ValueInteger *cycleDec;

		int cycleMoveRa;
		int cycleMoveDec;

		rts2core::ValueInteger *defVelRa;
		rts2core::ValueInteger *defVelDec;

		rts2core::ValueInteger *accRa;
		rts2core::ValueInteger *accDec;

		rts2core::ValueInteger *velRa;
		rts2core::ValueInteger *velDec;

		rts2core::ValueInteger *qRa;
		rts2core::ValueInteger *qDec;

		rts2core::ValueInteger *microRa;
		rts2core::ValueInteger *microDec;

		rts2core::ValueInteger *numberRa;
		rts2core::ValueInteger *numberDec;

		rts2core::ValueInteger *startRa;
		rts2core::ValueInteger *startDec;

		rts2core::ValueInteger *accWormRa;
		rts2core::ValueInteger *velWormRa;
		rts2core::ValueInteger *backWormRa;
		rts2core::ValueInteger *waitWormRa;

		int32_t ac, dc;

		int32_t info_u_ra;
		int32_t info_u_dec;

		int32_t last_off_ra;
		int32_t last_off_dec;

		int32_t last_move_ra;
		int32_t last_move_dec;

		void tel_run (rts2core::ConnSerial *conn, int value);
		/**
		 * Stop telescope movement. Phases is bit mask indicating which phase should be commited.
		 * First bit in phase is for sending Kill command, second is for waiting for reply.
		 */
		void tel_kill (rts2core::ConnSerial *conn, int phases = 0x03);

		void stopMoveRa ();
		void stopMoveDec ();

		void initMotors ();
		void initRa ();
		void initDec ();
};

}

using namespace rts2teld;


void Trencin::tel_write (rts2core::ConnSerial *conn, char command)
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

void Trencin::tel_write (rts2core::ConnSerial *conn, const char *command)
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

void Trencin::tel_write (rts2core::ConnSerial *conn, char command, int32_t value)
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
	int ret;
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
	ret = readAxis (trencinConnRa, unitRa, false);
	if (ret < 0)
		return -1;
	raWormStart->setNow ();
	worm_start_unit_ra = unitRa->getValueInteger ();
	sendValueAll (raWormStart);
	return 0;
}

int Trencin::stopWorm ()
{
	if (!isnan (raWormStart->getValueDouble ()))
	{
		tel_write_ra ('\\');
		worm_start_unit_ra += (getNow () - raWormStart->getValueDouble ()) * haCpd / (240.0 * LN_SIDEREAL_DAY_SEC / 86400);
		if (worm_start_unit_ra > MAX_MOVE)
			worm_start_unit_ra -= MAX_MOVE;
		if (worm_start_unit_ra < 0)
			worm_start_unit_ra += MAX_MOVE;
		tel_write_ra ('=', worm_start_unit_ra);
		// reset info and unit pointes..
		info_u_ra = worm_start_unit_ra;
		unitRa->setValueInteger (worm_start_unit_ra);
		// set proper values for speed after reset
		initRa ();
		raWormStart->setValueDouble (NAN);
		sendValueAll (raWormStart);
		deleteTimers (EVENT_TIMER_RA_WORM);
	}
	return 0;
}

void Trencin::addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	if (raMoving->getValueInteger () != 0 || !isnan (raWormStart->getValueDouble ()))
		trencinConnRa->add (&read_set, &write_set, &exp_set);
	if (decMoving->getValueInteger () != 0)
		trencinConnDec->add (&read_set, &write_set, &exp_set);
	Telescope::addSelectSocks (read_set, write_set, exp_set);
}

void Trencin::selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	// old axis value
	if ((raMoving->getValueInteger () != 0 || !isnan (raWormStart->getValueDouble ())) && trencinConnRa->receivedData (&read_set))
	{
		int old_axis = unitRa->getValueInteger ();
		if (readAxis (trencinConnRa, unitRa, false) == 0)
		{
#ifdef DEBUG_MOVE
			logStream (MESSAGE_DEBUG) << "selectSuccess cycleRa " << cycleRa->getValueInteger () << sendLog;
#endif
			if (!isnan (raWormStart->getValueDouble ()))
			{
				// moves backward..so if reading becomes higher, we crosed a full cycle..
				if (unitRa->getValueInteger () > old_axis && unitRa->getValueInteger () > (MAX_MOVE - 100000) && old_axis < 100000)
					cycleRa->dec ();
				trencinConnRa->flushPortIO ();
			}
			else if (fabs (raMoving->getValueInteger ()) > MAX_MOVE)
			{
				tel_run (trencinConnRa, raMoving->getValueInteger () - (raMoving->getValueInteger () > 0 ? MAX_MOVE : -MAX_MOVE));
			}
			else
			{
				raMoving->setValueInteger (0);
			}
			sendValueAll (raMoving);

#ifdef DEBUG_MOVE
			logStream (MESSAGE_DEBUG) << "selectSuccess cycleRa " << cycleRa->getValueInteger () << sendLog;
#endif
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
				tel_run (trencinConnDec, decMoving->getValueInteger () - (decMoving->getValueInteger () > 0 ? MAX_MOVE : -MAX_MOVE));
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

	Telescope::selectSuccess (read_set, write_set, exp_set);
}

int Trencin::readAxis (rts2core::ConnSerial *conn, rts2core::ValueInteger *value, bool write_axis)
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
		if (decGuide->getValueInteger () == 0)
			setIdleInfoInterval (60);
		if (wormRa->getValueBool () == true)
			startWorm ();
		return;
	}
	stopWorm ();
	switch (value)
	{
		case 1:
			raMoving->setValueInteger (MAX_MOVE);
			break;
		case 2:
			raMoving->setValueInteger (-MAX_MOVE);
			break;
	}
	cycleMoveRa = 0;
	tel_run (trencinConnRa, raMoving->getValueInteger ());
	setIdleInfoInterval (0.5);
}

void Trencin::setGuideDec (int value)
{
	stopMoveDec ();
	if (value == 0)
	{
		if (raGuide->getValueInteger () == 0)
			setIdleInfoInterval (60);
		return;
	}
	switch (value)
	{
		case 1:
			decMoving->setValueInteger (MAX_MOVE);
			break;
		case 2:
			decMoving->setValueInteger (-MAX_MOVE);
			break;
	}
	cycleMoveDec = 0;
	tel_run (trencinConnDec, decMoving->getValueInteger ());
	setIdleInfoInterval (0.5);
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

void Trencin::checkAcc (rts2core::ConnSerial *conn, rts2core::ValueInteger *acc, rts2core::ValueInteger *startStop)
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
	// apply offset for sidereal motion
	if (wormRa->getValueBool () == true)
	{
		switch (raGuide->getValueInteger ())
		{
			case 1:
				// in sidereal direction - we need to be quicker
				new_speed += 4;
				break;
			case 2:
				new_speed -= 4;
				break;
		}
	}
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
	sendValueAll (velRa);
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
	sendValueAll (velDec);
}

void Trencin::setRa (long new_ra)
{
#ifdef DEBUG_MOVE
	logStream (MESSAGE_DEBUG) << "setRa raMoving " << raMoving->getValueInteger () << sendLog;
#endif
	if (raMoving->getValueInteger () != 0)
	{
		tel_kill (trencinConnRa);
		raMovingEnd->setValueDouble (getNow ());
	}
	else
	{
		stopWorm ();
		sleep (3);
	}
	readAxis (trencinConnRa, unitRa);

	long diff = new_ra - unitRa->getValueInteger () - cycleRa->getValueInteger () * MAX_MOVE;
	long old_diff = diff;

	if (wormRa->getValueBool ())
	{
		// adjust for siderial move..
		double v = ((double) velRa->getValueInteger ()) * 64;
		// sideric speed in 1/64 steps / sec
		double sspeed = (haCpd / 240) * (86400 / LN_SIDEREAL_DAY_SEC);
		if ((sspeed < 0 && diff < 0) || (sspeed > 0 && diff > 0))
			// we are going in direction of sidereal motion, thus the motion will take shorter time
			sspeed = -1 * fabs (sspeed);
		else
			sspeed = fabs (sspeed);
		diff *= v / (v + sspeed);
		diff += 2 * sspeed;
	}

	logStream (MESSAGE_INFO) << "original diff " << old_diff << " adjusted diff " << diff << sendLog;
	cycleMoveRa = 0;
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

	long diff = new_dec - unitDec->getValueLong () - cycleDec->getValueInteger () * MAX_MOVE;
	cycleMoveDec = 0;
	tel_run (trencinConnDec, diff);
}

Trencin::Trencin (int _argc, char **_argv):Fork (_argc, _argv)
{
	trencinConnRa = NULL;
	trencinConnDec = NULL;

	parkPos = NULL;
	assumeParked = false;

	haZero = 0;
	decZero = 0;

	haCpd = -56889;
	decCpd = -119040;

	ra_ticks = (int32_t) (fabs (haCpd) * 360);
	dec_ticks = (int32_t) (fabs (decCpd) * 360);

	acMargin = (int32_t) (haCpd * 5);

	last_off_ra = 0;
	last_off_dec = 0;

	last_move_ra = 0;
	last_move_dec = 0;

	device_nameRa = "/dev/ttyS0";
	device_nameDec = "/dev/ttyS1";
	addOption ('r', NULL, 1, "device file for RA motor (default /dev/ttyS0)");
	addOption ('D', NULL, 1, "device file for DEC motor (default /dev/ttyS1)");
	addOption (OPT_PARK_POS, "park", 1, "parking position (alt az separated with :)");
	addOption (OPT_ASSSUME_0_PARK, "assume-parked", 0, "assume mount is parked when motors read 0 0");

	createRaGuide ();
	createDecGuide ();

	createValue (guidingSpeed, "guiding_speed", "guiding speed in deg/sec", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	guidingSpeed->setValueDouble (0.5);

	createValue (raMoving, "ra_moving", "if RA drive is moving", false);
	raMoving->setValueInteger (0);

	createValue (decMoving, "dec_moving", "if DEC drive is moving", false);
	decMoving->setValueInteger (0);

	createValue (raMovingEnd, "ra_moving_end", "time of end of last RA movement", false);
	createValue (decMovingEnd, "dec_moving_end", "time of end of last DEC movement", false);

	createValue (wormRa, "TRACKING", "RA worm drive", false, RTS2_VALUE_WRITABLE);
	wormRa->setValueBool (false);

	createValue (raWormStart, "ra_worm_start", "RA worm start time", false);
	raWormStart->setValueDouble (NAN);

	createValue (unitRa, "AXRA", "RA axis raw counts", true, RTS2_VALUE_WRITABLE);
	unitRa->setValueInteger (0);

	createValue (unitDec, "AXDEC", "DEC axis raw counts", true, RTS2_VALUE_WRITABLE);
	unitDec->setValueInteger (0);

	createValue (cycleRa, "cycle_ra", "number of full RA motor cycles", true, RTS2_VALUE_WRITABLE);
	cycleRa->setValueInteger (0);

	createValue (cycleDec, "cycle_dec", "number of full DEC motor cycles", true, RTS2_VALUE_WRITABLE);
	cycleDec->setValueInteger (0);

	cycleMoveRa = 0;
	cycleMoveDec = 0;

	createValue (defVelRa, "vel_ra", "RA maximal velocity", false, RTS2_VALUE_WRITABLE);
	createValue (defVelDec, "vel_dec", "DEC maximal velocity", false, RTS2_VALUE_WRITABLE);

	defVelRa->setValueInteger (1500);
	defVelDec->setValueInteger (1500);

	createValue (accRa, "acc_ra", "RA acceleration", false, RTS2_VALUE_WRITABLE);
	createValue (accDec, "acc_dec", "DEC acceleration", false, RTS2_VALUE_WRITABLE);

	accRa->setValueInteger (800);
	accDec->setValueInteger (800);

	createValue (velRa, "c_vel_ra", "RA current velocity", false, RTS2_VALUE_WRITABLE);
	createValue (velDec, "c_vel_dec", "DEC current velocity", false, RTS2_VALUE_WRITABLE);

	velRa->setValueInteger (defVelRa->getValueInteger ());
	velDec->setValueInteger (defVelDec->getValueInteger ());

	createValue (qRa, "qualification_ra", "number of microsteps in top speeds", false, RTS2_VALUE_WRITABLE);

	createValue (qDec, "qualification_dec", "number of microsteps in top speeds", false, RTS2_VALUE_WRITABLE);
	
	qRa->setValueInteger (2);
	qDec->setValueInteger (2);

	createValue (microRa, "micro_ra", "RA microstepping", false, RTS2_VALUE_WRITABLE);
	createValue (microDec, "micro_dec", "DEC microstepping", false, RTS2_VALUE_WRITABLE);

	microRa->setValueInteger (8);
	microDec->setValueInteger (8);

	createValue (numberRa, "number_ra", "current shape in microstepping", false, RTS2_VALUE_WRITABLE);
	createValue (numberDec, "number_dec", "current shape in microstepping", false, RTS2_VALUE_WRITABLE);

	numberRa->setValueInteger (6);
	numberDec->setValueInteger (6);

	createValue (startRa, "start_ra", "start/stop speed", false, RTS2_VALUE_WRITABLE);
	createValue (startDec, "start_dec", "start/stop speed", false, RTS2_VALUE_WRITABLE);

	startRa->setValueInteger (200);
	startDec->setValueInteger (200);

	createValue (accWormRa, "acc_worm_ra", "acceleration for worm in RA", false, RTS2_VALUE_WRITABLE);
	accWormRa->setValueInteger (100);

	createValue (velWormRa, "vel_worm_ra", "velocity for worm speeds in RA", false, RTS2_VALUE_WRITABLE);
	velWormRa->setValueInteger (200);

	createValue (backWormRa, "back_worm_ra", "backward worm trajectory", false, RTS2_VALUE_WRITABLE);
	backWormRa->setValueInteger (24);

	createValue (waitWormRa, "wait_worm_ra", "wait during RA worm cycle", false, RTS2_VALUE_WRITABLE);
	waitWormRa->setValueInteger (101);

	// apply all corrections
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
		case OPT_PARK_POS:
			{
				std::istringstream *is;
				is = new std::istringstream (std::string(optarg));
				double palt,paz;
				char c;
				*is >> palt >> c >> paz;
				if (is->fail () || c != ':')
				{
					logStream (MESSAGE_ERROR) << "Cannot parse alt-az park position " << optarg << sendLog;
					delete is;
					return -1;
				}
				delete is;
				if (parkPos == NULL)
					createValue (parkPos, "park_position", "mount park position", false);
				parkPos->setValueAltAz (palt, paz);
			}
			break;
		case OPT_ASSSUME_0_PARK:
			assumeParked = true;
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

int Trencin::setTo (double set_ra, double set_dec)
{
	int ret = stopMove ();
	if (ret)
		return ret;
	ret = stopWorm ();
	if (ret)
		return ret;
	// calculate expected RA and DEC ticsk..
	int32_t u_ra;
	int32_t u_dec;
	int32_t off;
	getHomeOffset (off);

	struct ln_equ_posn pos;
	pos.ra = set_ra;
	pos.dec = set_dec;

	setTarget (pos.ra, pos.dec);
	ret = sky2counts (u_ra, u_dec);
	if (ret)
		return -1;

	cycleRa->setValueInteger (u_ra / MAX_MOVE);
	cycleDec->setValueInteger (u_dec / MAX_MOVE);

	logStream (MESSAGE_INFO) << "setting to " << LibnovaRaDec (&pos) << " u_ra " << u_ra << " u_dec " << u_dec << " cycle ra dec " << cycleRa->getValueInteger () << " " << cycleDec->getValueInteger () << sendLog;

	u_ra %= MAX_MOVE;
	if (u_ra < 0)
	{
		u_ra += MAX_MOVE;
		cycleRa->dec ();
	}

	u_dec %= MAX_MOVE;
	if (u_dec < 0)
	{
		u_dec += MAX_MOVE;
		cycleDec->dec ();
	}

	try
	{
		tel_write_ra ('=', u_ra);
		tel_write_dec ('=', u_dec);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot set position " << er << sendLog;
		return -1;
	}

	logStream (MESSAGE_INFO) << "after setting to " << LibnovaRaDec (&pos) << " u_ra " << u_ra << " u_dec " << u_dec << " cycle ra dec " << cycleRa->getValueInteger () << " " << cycleDec->getValueInteger () << sendLog;

	cycleMoveRa = 0;
	cycleMoveDec = 0;

	last_move_ra = 0;
	last_move_dec = 0;

	if (wormRa->getValueBool () == true)
		return startWorm ();
	return 0;
}

int Trencin::init ()
{
	int ret;

	ret = Fork::init ();
	if (ret)
		return ret;

	rts2core::Configuration *config = rts2core::Configuration::instance ();
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

	trencinConnRa = new rts2core::ConnSerial (device_nameRa, this, rts2core::BS4800, rts2core::C8, rts2core::NONE, 40);
	ret = trencinConnRa->init ();
	if (ret)
		return ret;

	trencinConnRa->setDebug ();
	trencinConnRa->flushPortIO ();

	trencinConnDec = new rts2core::ConnSerial (device_nameDec, this, rts2core::BS4800, rts2core::C8, rts2core::NONE, 40);
	ret = trencinConnDec->init ();
	if (ret)
		return ret;

	trencinConnDec->setDebug ();
	trencinConnDec->flushPortIO ();

	snprintf (telType, 64, "Trencin");

	try
	{
		tel_write_ra ('K');
		tel_write_dec ('K');
		initMotors ();
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot init motors" << sendLog;
		return -1;
	}

	// if parked..
	if (assumeParked)
	{
		struct ln_hrz_posn hrz;
		struct ln_equ_posn equ;

		hrz.alt = parkPos->getAlt ();
		hrz.az = parkPos->getAz ();

		ln_get_equ_from_hrz (&hrz, config->getObserver (), ln_get_julian_from_sys (), &equ);

		setTo (equ.ra, equ.dec);
		maskState (TEL_MASK_MOVING, TEL_PARKED, "telescope was probably parked at initializiation");
		logStream (MESSAGE_DEBUG) << "set telescope to park position (" << LibnovaHrz (&hrz) << ")" << sendLog;
	}

	return ret;
}

int Trencin::updateLimits ()
{
	acMin = (int32_t) (fabs (haCpd) * -180);
	acMax = (int32_t) (fabs (haCpd) * 180);

	return 0;
}

void Trencin::updateTrack ()
{
}

int Trencin::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
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
			if (((rts2core::ValueBool *)new_value)->getValueBool () == true)
			{
				if (raMoving->getValueInteger () == 0)
					startWorm ();
			}
			else
			{
				stopWorm ();
			}
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

void Trencin::valueChanged (rts2core::Value *changed_value)
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
	double t_telRa;
	double t_telDec;

	int32_t u_ra, u_dec;

	int32_t left_track;

	// update axRa and axDec
	if (isnan (raWormStart->getValueDouble ()))
	{
#ifdef DEBUG_MOVE
		logStream (MESSAGE_DEBUG) << "cycleRa " << cycleRa->getValueInteger () << " cycleMoveRa " << cycleMoveRa << sendLog;
#endif
		if (raMoving->getValueInteger () == 0)
		{
			readAxis (trencinConnRa, unitRa);
#ifdef DEBUG_MOVE
			logStream (MESSAGE_DEBUG) << "unitRa " << unitRa->getValueInteger () << " last_move_ra " << last_move_ra << sendLog;
#endif
			u_ra = unitRa->getValueInteger ();
			if (cycleMoveRa == 0)
			{
				if (u_ra < info_u_ra && last_move_ra > 0)
				{
					cycleRa->inc ();
					cycleMoveRa--;
				}
				if (u_ra > info_u_ra && last_move_ra < 0)
				{
					cycleRa->dec ();
					cycleMoveRa++;
				}

				sendValueAll (cycleRa);
			}
	 		u_ra += MAX_MOVE * cycleRa->getValueInteger ();
#ifdef DEBUG_MOVE
			logStream (MESSAGE_DEBUG) << "u_ra " << u_ra << " cycleRa " << cycleRa->getValueInteger () << sendLog;
#endif
		}
		else
		{
			left_track = velRa->getValueInteger () * 64 * (raMovingEnd->getValueDouble () - getNow ());
			// sometime we are too fast in calculating left track - it must be smaller then total, which is in raMoving
			if (left_track > fabs (raMoving->getValueInteger ()))
				left_track = fabs (raMoving->getValueInteger ());
			info_u_ra = MAX_MOVE * cycleMoveRa + unitRa->getValueInteger () + raMoving->getValueInteger () * (1 - (double) left_track / fabs (raMoving->getValueInteger ()));
#ifdef DEBUG_MOVE
			logStream (MESSAGE_DEBUG) << "cycleRa " << cycleRa->getValueInteger () << " info_u_ra " << info_u_ra << " raMoving " << raMoving->getValueInteger () << " unitRa " << unitRa->getValueInteger () << " left_track " << left_track << sendLog;
#endif
			if (info_u_ra < 0)
			{
				cycleRa->dec ();
				cycleMoveRa++;
				info_u_ra += MAX_MOVE;
			}
			if (info_u_ra > MAX_MOVE)
			{
				cycleRa->inc ();
				cycleMoveRa--;
				info_u_ra -= MAX_MOVE;
			}

			u_ra = MAX_MOVE * cycleRa->getValueInteger () + info_u_ra;
		}
#ifdef DEBUG_MOVE
		logStream (MESSAGE_DEBUG) << "cycleRa " << cycleRa->getValueInteger () << " info_u_ra " << info_u_ra << " u_ra " << u_ra << sendLog;
#endif
	}
	else
	{
		// RA worm is running, unitRa and cycleRa are set when new position arrives from the axis
		u_ra = MAX_MOVE * cycleRa->getValueInteger () + unitRa->getValueInteger ();
	}

	if (decMoving->getValueInteger () == 0)
	{
		readAxis (trencinConnDec, unitDec);
 		u_dec = unitDec->getValueInteger ();
		if (cycleMoveDec == 0)
		{
			if (u_dec < info_u_dec && last_move_dec > 0)
			{
				cycleDec->inc ();
				cycleMoveDec--;
			}
			if (u_dec > info_u_dec && last_move_dec < 0)
			{
				cycleDec->dec ();
				cycleMoveDec++;
			}

			sendValueAll (cycleDec);
		}
		u_dec += MAX_MOVE * cycleDec->getValueInteger ();
	}
	else
	{
		left_track = velDec->getValueInteger () * 64 * (decMovingEnd->getValueDouble () - getNow ());
		if (left_track > fabs (decMoving->getValueInteger ()))
			left_track = fabs (decMoving->getValueInteger ());
		info_u_dec = MAX_MOVE * cycleMoveDec + unitDec->getValueInteger () + decMoving->getValueInteger () * (1 - (double) left_track / fabs (decMoving->getValueInteger ()));
		if (info_u_dec < 0)
		{
			cycleDec->dec ();
			cycleMoveDec++;
			info_u_dec += MAX_MOVE;
		}
		if (info_u_dec > MAX_MOVE)
		{
			cycleDec->inc ();
			cycleMoveDec--;
			info_u_dec -= MAX_MOVE;
		}

		u_dec = MAX_MOVE * cycleDec->getValueInteger () + info_u_dec;
	}
#ifdef DEBUG_MOVE
	logStream (MESSAGE_DEBUG) << "cycleDec " << cycleDec->getValueInteger () << " info_u_dec " << info_u_dec << " u_dec " << u_dec << sendLog;
#endif

	if (decMoving->getValueInteger () == 0 && raMoving->getValueInteger () == 0 && isnan (raWormStart->getValueDouble ()))
		setIdleInfoInterval (60);

	counts2sky (u_ra, u_dec, t_telRa, t_telDec);
	setTelRaDec (t_telRa, t_telDec);

	return Fork::info ();
}

int Trencin::startResync ()
{
	// calculate new X and Y..
	int ret;

	stopMove ();

	ret = sky2counts (ac, dc);
	if (ret)
		return -1;
	try
	{
	 	setSpeedRa (defVelRa->getValueInteger ());
		setSpeedDec (defVelDec->getValueInteger ());

		initRa ();
		initDec ();

		setRa (ac);
		setDec (dc);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}

	last_off_ra = 0;
	last_off_dec = 0;

	return 0;
}

int Trencin::isMoving ()
{
	if (raMoving->getValueInteger () != 0 || decMoving->getValueInteger () != 0)
		return USEC_SEC;
	return -2;
}

int Trencin::endMove ()
{
	info_u_ra = unitRa->getValueInteger ();
	info_u_dec = unitDec->getValueInteger ();
#ifdef DEBUG_MOVE
	logStream (MESSAGE_DEBUG) << "endMove cycleMoveRa " << cycleMoveRa << " cycleMoveDec " << cycleMoveDec << ", setting both to 0" << sendLog;
#endif
	cycleMoveRa = 0;
	cycleMoveDec = 0;

	raMoving->setValueInteger (0);
	decMoving->setValueInteger (0);

	if (wormRa->getValueBool () == true)
		startWorm ();
	return Fork::endMove ();
}

int Trencin::stopMove ()
{
	try 
	{
		tel_kill (trencinConnRa, 0x01);
		tel_kill (trencinConnDec, 0x01);

		tel_kill (trencinConnRa, 0x02);
		tel_kill (trencinConnDec, 0x02);

		raMoving->setValueInteger (0);
		decMoving->setValueInteger (0);

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

void Trencin::startOffseting (rts2core::Value *changed_value)
{
	int32_t new_ra_off = haCpd * getOffsetRa ();
	int32_t new_dec_off = decCpd * getOffsetDec ();
	bool moved = false;

	if (new_ra_off != last_off_ra)
	{
		if (raMoving->getValueInteger () != 0)
		{
			tel_kill (trencinConnRa);
			raMovingEnd->setValueDouble (getNow ());
		}
		else
		{
			stopWorm ();
			usleep (USEC_SEC * 1.5);
		}
		tel_run (trencinConnRa, new_ra_off - last_off_ra);
		last_off_ra = new_ra_off;
		setIdleInfoInterval (0.5);
		moved = true;
	}

	if (new_dec_off != last_off_dec)
	{
		if (decMoving->getValueInteger () != 0)
		{
			tel_kill (trencinConnDec);
			decMovingEnd->setValueDouble (getNow ());
		}
		tel_run (trencinConnDec, new_dec_off - last_off_dec);
		last_off_dec = new_dec_off;
		setIdleInfoInterval (0.5);
		moved = true;
	}

	if (moved)
		maskState (TEL_MASK_MOVING, TEL_MOVING, "offseting started");
}

int Trencin::startPark ()
{
	try
	{
		if ((getState () & TEL_MASK_MOVING) == TEL_PARKED || (getState () & TEL_MASK_MOVING) == TEL_PARKING)
			return 0;
		if (wormRa->getValueBool ())
		{
			stopWorm ();
			sleep (3);
			wormRa->setValueBool (true);
		}
		initRa ();
		initDec ();

		// calculate parking ra dec
		setTargetAltAz (parkPos->getAlt (), parkPos->getAz ());
		return moveAltAz ();

	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "canot start park " << er << sendLog;
		return -1;
	}
}

int Trencin::isParking ()
{
	return isMoving ();
}

int Trencin::endPark ()
{
	return 0;
}

int Trencin::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("reset"))
	{
		try
		{
			tel_write_ra ('\\');
			tel_write_dec ('\\');
			cycleRa->setValueInteger (0);
			cycleDec->setValueInteger (0);
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

void Trencin::tel_run (rts2core::ConnSerial *conn, int value)
{
#ifdef DEBUG_MOVE
	logStream (MESSAGE_DEBUG) << "tel_run " << (conn == trencinConnRa) << sendLog;
#endif
	if (value == 0)
		return;

	conn->flushPortIO ();

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
		last_move_ra = value;

		sendValueAll (raMovingEnd);
		sendValueAll (raMoving);
	}
	else if (conn == trencinConnDec)
	{
	  	decMovingEnd->setValueDouble (getNow () + 2 + fabs (value) / (64 * velDec->getValueInteger ()));
		decMoving->setValueInteger (value);
		last_move_dec = value;

		sendValueAll (decMovingEnd);
		sendValueAll (decMoving);
	}
}

void Trencin::tel_kill (rts2core::ConnSerial *conn, int phases)
{
#ifdef DEBUG_MOVE
	logStream (MESSAGE_DEBUG) << "kill cycleRa " << cycleRa->getValueInteger () << " cycleDec " << cycleDec->getValueInteger () << sendLog;
#endif 
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
		conn->setVTime (40);
		conn->flushPortIO ();

		rts2core::ValueInteger *unit;
		rts2core::ValueInteger *cycle;
		int cycleMove;
		int32_t last_u;


		if (conn == trencinConnRa)
		{
			unit = unitRa;
			cycle = cycleRa;
			cycleMove = cycleMoveRa;
			last_u = info_u_ra;

			cycleMoveRa = 0;
		}
		else if (conn == trencinConnDec)
		{
			unit = unitDec;
			cycle = cycleDec;
			cycleMove = cycleMoveDec;
			last_u = info_u_dec;

			cycleMoveDec = 0;
		}
		else
		{
			throw rts2core::Error ("unexpected axis connection passed to tel_kill");
		}

		// read current value and check for expected cycle value - if it does not agree, change it
		readAxis (conn, unit);

#ifdef DEBUG_MOVE
		logStream (MESSAGE_DEBUG) << "kill cycleMove " << cycleMove << " cycle " << cycle->getValueInteger () << " unit " << unit->getValueInteger () << " last " << last_u << sendLog;
#endif

		// last_move_xx < 0
		if (cycleMove > 0 && unit->getValueInteger () < last_u)
			cycle->inc ();
		// last_move_xx > 0
		else if (cycleMove < 0 && unit->getValueInteger () > last_u)
			cycle->dec ();
	}
#ifdef DEBUG_MOVE
	logStream (MESSAGE_DEBUG) << "kill ends cycleRa " << cycleRa->getValueInteger () << " cycleDec " << cycleDec->getValueInteger () << sendLog;
#endif 
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
