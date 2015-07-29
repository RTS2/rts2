/* 
 * Sidereal Technology Controller driver
 * Copyright (C) 2012-2013 Shashikiran Ganesh <shashikiran.ganesh@gmail.com>
 * Copyright (C) 2014-2015 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2014-2015 ISDEFE/ESA
 * Based on John Kielkopf's xmtel linux software, Dan's SiTech driver and documentation, and dummy telescope driver.
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

#include "gem.h"
#include "configuration.h"

#include "sitech.h"
#include "connection/sitech.h"

#define RTS2_SITECH_TIMER       RTS2_LOCAL_EVENT + 1220
#define TRACK_TIME_DELTA        1.0

// SITech counts/servero loop -> speed value
#define SPEED_MULTI             65536

// Crystal frequency
#define CRYSTAL_FREQ            96000000

namespace rts2teld
{

/**
 * Sidereal Technology GEM telescope driver.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Sitech:public GEM
{
	public:
		Sitech (int argc, char **argv);
		~Sitech (void);

		virtual void postEvent (rts2core::Event *event);

	protected:
		virtual int processOption (int in_opt);

		virtual int commandAuthorized (rts2core::Connection *conn);

		virtual int initValues ();
		virtual int initHardware ();

		virtual int info ();

		virtual int startResync ();

		virtual int endMove ();

		virtual int stopMove ()
		{
			fullStop();
			return 0;
		}

		virtual int startPark ();

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

		virtual int setTracking (bool track);

		virtual void setDiffTrack (double dra, double ddec);

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int updateLimits ();

		/**
		 * Gets home offset.
		 */
		virtual int getHomeOffset (int32_t & off)
		{
			off = 0;
			return 0;
		}

	private:
		void getConfiguration ();

		/**
		 * Sends SiTech XYS command with requested coordinates.
		 */
		int sitechMove (int32_t ac, int32_t dc);

		/**
		 * Starts mount tracking - endless speed limited pointing.
                 * Called periodically to make sure we stay on target.
		 */
		void sitechStartTracking (bool startTimer);

		// speed conversion; see Dan manual for details
		double degsPerSec2MotorSpeed (double dps, int32_t loop_ticks, double full_circle = SIDEREAL_HOURS * 15.0);
		double ticksPerSec2MotorSpeed (double tps);
		double motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks);

		ConnSitech *serConn;

		SitechAxisStatus radec_status;
		SitechYAxisRequest radec_Yrequest;
		SitechXAxisRequest radec_Xrequest;

		rts2core::ValueDouble *sitechVersion;
		rts2core::ValueInteger *sitechSerial;

		rts2core::ValueLong *ra_pos;
		rts2core::ValueLong *dec_pos;

		rts2core::ValueLong *r_ra_pos;
		rts2core::ValueLong *r_dec_pos;

		rts2core::ValueLong *t_ra_pos;
		rts2core::ValueLong *t_dec_pos;

		rts2core::ValueInteger *partialMove;

		rts2core::ValueDouble *trackingDist;

		rts2core::IntegerArray *PIDs;

		rts2core::ValueLong *ra_enc;
		rts2core::ValueLong *dec_enc;

		rts2core::ValueLong *mclock;
		rts2core::ValueInteger *temperature;

		rts2core::ValueInteger *ra_worm_phase;

		rts2core::ValueLong *ra_last;
		rts2core::ValueLong *dec_last;

		rts2core::ValueString *ra_errors;
		rts2core::ValueString *dec_errors;

		rts2core::ValueInteger *ra_errors_val;
		rts2core::ValueInteger *dec_errors_val;

		rts2core::ValueInteger *ra_pos_error;
		rts2core::ValueInteger *dec_pos_error;

		rts2core::ValueInteger *ra_supply;
		rts2core::ValueInteger *dec_supply;

		rts2core::ValueInteger *ra_temp;
		rts2core::ValueInteger *dec_temp;

		rts2core::ValueInteger *ra_pid_out;
		rts2core::ValueInteger *dec_pid_out;

		// request values - speed,..
		rts2core::ValueDouble *ra_speed;
		rts2core::ValueDouble *dec_speed;

		// tracking speed in controller units
		rts2core::ValueDouble *ra_track_speed;
		rts2core::ValueDouble *dec_track_speed;

		rts2core::ValueLong *ra_sitech_speed;
		rts2core::ValueLong *dec_sitech_speed;

		rts2core::ValueBool *use_constant_speed;

		// current PID values
		rts2core::ValueBool *trackingPIDs;

		rts2core::ValueDouble *ra_acceleration;
		rts2core::ValueDouble *dec_acceleration;

		rts2core::ValueDouble *ra_max_velocity;
		rts2core::ValueDouble *dec_max_velocity;

		rts2core::ValueDouble *ra_current;
		rts2core::ValueDouble *dec_current;

		rts2core::ValueInteger *ra_pwm;
		rts2core::ValueInteger *dec_pwm;

		rts2core::ValueInteger *countUp;
		rts2core::ValueDouble *servoPIDSampleRate;

		double offsetha;
		double offsetdec;
                 
		int pmodel;                    /* pointing model default to raw data */
	
		/* Communications variables and routines for internal use */
		const char *device_file;

		void fullStop ();

		// JD used in last getTel call
		double getTelJD;
		
		/**
		 * Retrieve telescope counts, convert them to RA and Declination.
		 */
		void getTel ();
		void getTel (double &telra, double &teldec, int &telflip, double &un_telra, double &un_teldec);

		/**
		 * Retrieves current PID settings
		 */
		void getPIDs ();

		// which controller is connected
		enum {SERVO_I, SERVO_II, FORCE_ONE} sitechType;

		uint8_t xbits;
		uint8_t ybits;

		bool wasStopped;

		std::string findErrors (uint16_t e);
};

}

using namespace rts2teld;

Sitech::Sitech (int argc, char **argv):GEM (argc, argv, true, true), radec_status (), radec_Yrequest (), radec_Xrequest ()
{
	unlockPointing ();

	setCorrections (true, true, true);

	offsetha = 0.;
	offsetdec = 0.;

	pmodel = RAW;

	device_file = "/dev/ttyUSB0";

	acMargin = 0;

	wasStopped = false;

	createValue (sitechVersion, "sitech_version", "SiTech controller firmware version", false);
	createValue (sitechSerial, "sitech_serial", "SiTech controller serial number", false);

	createValue (ra_pos, "AXRA", "RA motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (dec_pos, "AXDEC", "DEC motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (r_ra_pos, "R_AXRA", "real RA motor axis count", true);
	createValue (r_dec_pos, "R_AXDEC", "real DEC motor axis count", true);

	createValue (t_ra_pos, "T_AXRA", "target RA motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (t_dec_pos, "T_AXDEC", "target DEC motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (partialMove, "partial_move", "1 - RA only, 2 - DEC only, move just close to the horizon", false);
	partialMove->setValueInteger (0);

	createValue (trackingDist, "tracking_dist", "tracking error budged (bellow this value, telescope will start tracking", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);

	createValue (PIDs, "pids", "axis PID values", false);

	// default to 1 arcsec
	trackingDist->setValueDouble (1 / 60.0 / 60.0);

	createValue (ra_enc, "ENCRA", "RA encoder readout", true);
	createValue (dec_enc, "ENCDEC", "DEC encoder readout", true);

	createValue (mclock, "mclock", "millisecond board clocks", false);
	createValue (temperature, "temperature", "[C] board temperature (CPU)", false);
	createValue (ra_worm_phase, "y_worm_phase", "RA worm phase", false);

	createValue (ra_last, "ra_last", "RA motor location at last RA scope encoder location change", false);
	createValue (dec_last, "dec_last", "DEC motor location at last DEC scope encoder location change", false);

	createValue (ra_errors, "ra_errors", "RA errors (only for FORCE ONE)", false);
	createValue (dec_errors, "dec_erorrs", "DEC errors (only for FORCE_ONE)", false);

	createValue (ra_errors_val, "ra_errors_val", "RA errors value", false);
	createValue (dec_errors_val, "dec_erorrs_val", "DEC errors value", false);

	createValue (ra_pos_error, "ra_pos_error", "RA positional error", false);
	createValue (dec_pos_error, "dec_pos_error", "DEC positional error", false);

	createValue (ra_supply, "ra_supply", "[V] RA supply", false);
	createValue (dec_supply, "dec_supply", "[V] DEC supply", false);

	createValue (ra_temp, "ra_temp", "[F] RA power board CPU temperature", false);
	createValue (dec_temp, "dec_temp", "[F] DEC power board CPU temperature", false);

	createValue (ra_pid_out, "ra_pid_out", "RA PID output", false);
	createValue (dec_pid_out, "dec_pid_out", "DEC PID output", false);

	createValue (ra_speed, "ra_speed", "[deg/s] RA speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	createValue (dec_speed, "dec_speed", "[deg/s] DEC speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);

	createValue (ra_track_speed, "ra_track_speed", "RA tracking speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE);
	createValue (dec_track_speed, "dec_track_speed", "DEC tracking speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE);

	createValue (ra_sitech_speed, "ra_sitech_speed", "speed in controller units", false);
	createValue (dec_sitech_speed, "dec_sitech_speed", "speed in controller units", false);

	createValue (use_constant_speed, "use_constant_speed", "use precalculated speed for corrections", false, RTS2_VALUE_WRITABLE);
	use_constant_speed->setValueBool (false);

	createValue (trackingPIDs, "tracking_pids", "true if tracking PIDs are in action", false);

	createValue (ra_acceleration, "ra_acceleration", "[deg/s^2] RA motor acceleration", false);
	createValue (dec_acceleration, "dec_acceleration", "[deg/s^2] DEC motor acceleration", false);

	createValue (ra_max_velocity, "ra_max_v", "[deg/s] RA axis maximal speed", false, RTS2_DT_DEGREES);
	createValue (dec_max_velocity, "dec_max_v", "[deg/s] RA axis maximal speed", false, RTS2_DT_DEGREES);

	createValue (ra_current, "ra_current", "[A] RA motor current", false);
	createValue (dec_current, "dec_current", "[A] DEC motor current", false);

	createValue (ra_pwm, "ra_pwm", "[W?] RA motor PWM output", false);
	createValue (dec_pwm, "dec_pwm", "[W?] DEC motor PWM output", false);

	ra_speed->setValueDouble (1);
	dec_speed->setValueDouble (1);

	ra_track_speed->setValueDouble (0);
	dec_track_speed->setValueDouble (0);

	createParkPos (0, 89.999, 0);

	addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");

	addParkPosOption ();
}

Sitech::~Sitech(void)
{
	delete serConn;
	serConn = NULL;
}

void Sitech::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case RTS2_SITECH_TIMER:
			if (wasStopped)
				break;
			sitechStartTracking (false);
			addTimer (TRACK_TIME_DELTA, event);
			return;
			break;
	}
	GEM::postEvent (event);
}

/* Full stop */
void Sitech::fullStop (void)
{
	partialMove->setValueInteger (0);
	try
	{
		serConn->siTechCommand ('X', "N");
		usleep (100000);
		serConn->siTechCommand ('Y', "N");

		wasStopped = true;
		deleteTimers (RTS2_SITECH_TIMER);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "cannot send full stop " << er << sendLog;
	}
}

void Sitech::getTel ()
{
	serConn->getAxisStatus ('X', radec_status);

	r_ra_pos->setValueLong (radec_status.y_pos);
	r_dec_pos->setValueLong (radec_status.x_pos);

	ra_pos->setValueLong (radec_status.y_pos + haZero->getValueDouble () * haCpd->getValueDouble ());
	dec_pos->setValueLong (radec_status.x_pos + decZero->getValueDouble () * decCpd->getValueDouble ());

	ra_enc->setValueLong (radec_status.y_enc);
	dec_enc->setValueLong (radec_status.x_enc);

	mclock->setValueLong (radec_status.mclock);
	temperature->setValueInteger (radec_status.temperature);

	ra_worm_phase->setValueInteger (radec_status.y_worm_phase);

	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			ra_last->setValueLong (le32toh (*(uint32_t*) &radec_status.y_last));
			dec_last->setValueLong (le32toh (*(uint32_t*) &radec_status.x_last));
			break;
		case FORCE_ONE:
		{
			// upper nimble
			uint16_t ra_val = radec_status.y_last[0] << 4;
			ra_val += radec_status.y_last[1];

			uint16_t dec_val = radec_status.x_last[0] << 4;
			dec_val += radec_status.x_last[1];

			// find all possible errors
			switch (radec_status.y_last[0] & 0x0F)
			{
				case 0:
					ra_errors_val->setValueInteger (ra_val);
					ra_errors->setValueString (findErrors (ra_val));
					break;

				case 1:
					ra_current->setValueInteger (ra_val);
					break;

				case 2:
					ra_supply->setValueInteger (ra_val);
					break;

				case 3:
					ra_temp->setValueInteger (ra_val);
					break;

				case 4:
					ra_pid_out->setValueInteger (ra_val);
					break;
			}


			switch (radec_status.x_last[0] & 0x0F)
			{
				case 0:
					dec_errors_val->setValueInteger (dec_val);
					dec_errors->setValueString (findErrors (dec_val));
					break;

				case 1:
					dec_current->setValueInteger (dec_val);
					break;

				case 2:
					dec_supply->setValueInteger (dec_val);
					break;

				case 3:
					dec_temp->setValueInteger (dec_val);
					break;

				case 4:
					dec_pid_out->setValueInteger (dec_val);
					break;
			}
			break;

			ra_pos_error->setValueInteger (*(uint16_t*) &radec_status.y_last[2]);
			dec_pos_error->setValueInteger (*(uint16_t*) &radec_status.x_last[2]);
		}
	}

	xbits = radec_status.x_bit;
	ybits = radec_status.y_bit;
}

void Sitech::getTel (double &telra, double &teldec, int &telflip, double &un_telra, double &un_teldec)
{
	getTelJD = ln_get_julian_from_sys ();

	getTel ();

	int ret = counts2sky (radec_status.y_pos, radec_status.x_pos, telra, teldec, telflip, un_telra, un_teldec, getTelJD);
	if (ret)
		logStream  (MESSAGE_ERROR) << "error transforming counts" << sendLog;
}

void Sitech::getPIDs ()
{
	PIDs->clear ();
	
	PIDs->addValue (serConn->getSiTechValue ('X', "PPP"));
	PIDs->addValue (serConn->getSiTechValue ('X', "III"));
	PIDs->addValue (serConn->getSiTechValue ('X', "DDD"));
}

std::string Sitech::findErrors (uint16_t e)
{
	std::string ret;
	if (e & 0x001)
		ret += "Gate Volts Low. ";
	if (e & 0x002)
		ret += "OverCurrent Hardware. ";
	if (e & 0x004)
		ret += "OverCurrent Firmware. ";
	if (e & 0x008)
		ret += "Motor Volts Low. ";
	if (e & 0x010)
		ret += "Power Board Over Temp. ";
	if (e & 0x020)
		ret += "Needs Reset (may not have any other errors). ";
	if (e & 0x040)
		ret += "Limit -. ";
	if (e & 0x080)
		ret += "Limit +. ";
	if (e & 0x100)
		ret += "Timed Over Current. ";
	if (e & 0x200)
		ret += "Position Error. ";
	if (e & 0x400)
		ret += "BiSS Encoder Error. ";
	if (e & 0x800)
		ret += "Checksum Error in return from Power Board. ";
	return ret;
}

/*!
 * Init telescope, connect on given tel_desc.
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int Sitech::initHardware ()
{
	int ret;
	int numread;

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();

	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());

	/* Make the connection */
	
	/* On the Sidereal Technologies controller                                */
	/*   there is an FTDI USB to serial converter that appears as             */
	/*   /dev/ttyUSB0 on Linux systems without other USB serial converters.   */
	/*   The serial device is known to the program that calls this procedure. */
	
	serConn = new ConnSitech (device_file, this);
	serConn->setDebug (getDebug ());

	ret = serConn->init ();

	if (ret)
		return -1;
	serConn->flushPortIO ();

	xbits = serConn->getSiTechValue ('X', "B");
	ybits = serConn->getSiTechValue ('Y', "B");

	numread = serConn->getSiTechValue ('X', "V");
     
	if (numread != 0) 
	{
		logStream (MESSAGE_DEBUG) << "Sidereal Technology Controller version " << numread / 10.0 << sendLog;
	}
	else
	{
		logStream (MESSAGE_ERROR) << "A200HR drive control did not respond." << sendLog;
		return -1;
	}

	sitechVersion->setValueDouble (numread);
	sitechSerial->setValueInteger (serConn->getSiTechValue ('Y', "V"));

	if (numread < 112)
	{
		sitechType = SERVO_II;
	}
	else
	{
		sitechType = FORCE_ONE;

		createValue (countUp, "count_up", "CPU count up", false);
		countUp->setValueInteger (serConn->getSiTechValue ('Y', "XHC"));

		createValue (servoPIDSampleRate, "servo_pid_sample_rate", "number of CPU cycles per second", false);
		servoPIDSampleRate->setValueDouble ((CRYSTAL_FREQ / 12.0) / (SPEED_MULTI - countUp->getValueInteger ()));

		getPIDs ();
	}

	//SitechControllerConfiguration sconfig;
	//serConn->getConfiguration (sconfig);

	//serConn->resetController ();
	
	/* Flush the input buffer in case there is something left from startup */

	serConn->flushPortIO();

	getConfiguration ();

	return 0;

}

int Sitech::info ()
{
	double t_telRa, t_telDec, ut_telRa, ut_telDec;
	int t_telFlip = 0;

	getTel (t_telRa, t_telDec, t_telFlip, ut_telRa, ut_telDec);

	setTelRaDec (t_telRa, t_telDec);
	telFlip->setValueInteger (t_telFlip);
	setTelUnRaDec (ut_telRa, ut_telDec);

	return rts2teld::GEM::info ();
}

int Sitech::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;

		default:
			return GEM::processOption (in_opt);
	}
	return 0;
}

int Sitech::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("zero_motor"))
	{
		if (!conn->paramEnd ())
			return -2;
		serConn->setSiTechValue ('X', "F", 0);
		serConn->setSiTechValue ('Y', "F", 0);
		return 0;
	}
	else if (conn->isCommand ("reset_errors"))
	{
		if (!conn->paramEnd ())
			return -2;
		serConn->resetErrors ();
		return 0;
	}
	else if (conn->isCommand ("reset_controller"))
	{
		if (!conn->paramEnd ())
			return -2;
		serConn->resetController ();
		getConfiguration ();
		return 0;
	}
	else if (conn->isCommand ("go_auto"))
	{
		if (!conn->paramEnd ())
			return -2;
		serConn->siTechCommand ('X', "A");
		serConn->siTechCommand ('Y', "A");
		getConfiguration ();
		return 0;
	}
	else if (conn->isCommand ("si_track"))
	{
		if (!conn->paramEnd ())
			return -2;
		sitechStartTracking (true);
		return 0;
	}
	else if (conn->isCommand ("si_strack"))
	{
		if (!conn->paramEnd ())
			return -2;
		ra_track_speed->setValueDouble (degsPerSec2MotorSpeed (15.0 / 3600.0, ra_ticks->getValueLong ()));
		dec_track_speed->setValueDouble (0);
		sendValueAll (ra_track_speed);
		sendValueAll (dec_track_speed);
		sitechStartTracking (true);
		return 0;
	}
	return GEM::commandAuthorized (conn);
}

int Sitech::initValues ()
{
	return GEM::initValues ();
}

int Sitech::startResync ()
{
	deleteTimers (RTS2_SITECH_TIMER);

	getConfiguration ();

	int32_t ac = r_ra_pos->getValueLong (), dc = r_dec_pos->getValueLong ();
	int ret = sky2counts (ac, dc);
	if (ret)
		return -1;

	wasStopped = false;

	t_ra_pos->setValueLong (ac);
	t_dec_pos->setValueLong (dc);

	ret = sitechMove (ac, dc);
	if (ret < 0)
		return ret;

	partialMove->setValueInteger (ret);
	sendValueAll (partialMove);

	return 0;
}

int Sitech::isMoving ()
{
	if (wasStopped)
		return -1;

	time_t now = time (NULL);
	if ((getTargetStarted () + 60) > now)
	{
		logStream (MESSAGE_ERROR) << "finished move due to timeout, target position not reached" << sendLog;
		return -1;
	}

	// if resync was only partial..
	if (partialMove->getValueInteger ())
	{
		// check if we are close enough to partial target values
		if ((labs (t_ra_pos->getValueLong () - r_ra_pos->getValueLong ()) < haCpd->getValueLong ()) && (labs (t_dec_pos->getValueLong () - r_dec_pos->getValueLong ()) < decCpd->getValueLong ()))
		{
			// try new move
			startResync ();
		}
		return USEC_SEC / 10;
	}

	if (getTargetDistance () > trackingDist->getValueDouble ())
	{
		// if too far away, update target
		startResync ();
		// close to target, increase update frequvency..
		if (getTargetDistance () < 0.5)
			return 0;

		return USEC_SEC / 10;
	}

	// set ra speed to sidereal tracking
	ra_track_speed->setValueDouble (degsPerSec2MotorSpeed (15.0 / 3600.0, ra_ticks->getValueLong ()));
	sendValueAll (ra_track_speed);

	return -2;
}

int Sitech::endMove ()
{
	partialMove->setValueInteger (0);
	setTracking (true);
	return GEM::endMove ();
}

int Sitech::setTracking (bool track)
{
	if (track)
	{
		wasStopped = false;
		sitechStartTracking (true);
	}
	else
	{
		fullStop ();
	}
	return GEM::setTracking (track);
}

void Sitech::setDiffTrack (double dra, double ddec)
{
	// convert to counts / hour
}

int Sitech::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == ra_pos)
	{
		partialMove->setValueInteger (0);
		sitechMove (newValue->getValueLong () - haZero->getValueDouble () * haCpd->getValueDouble (), dec_pos->getValueLong () - decZero->getValueDouble () * decCpd->getValueDouble ());
		return 0;
	}
	if (oldValue == dec_pos)
	{
		partialMove->setValueInteger (0);
		sitechMove (ra_pos->getValueLong () - haZero->getValueDouble () * haCpd->getValueDouble (), newValue->getValueLong () - decZero->getValueDouble () * decCpd->getValueDouble ());
		return 0;
	}

	return GEM::setValue (oldValue, newValue);
}

int Sitech::updateLimits ()
{
//	acMin->setValueLong (-20000000);
//	acMax->setValueLong (20000000);
//	dcMin->setValueLong (-20000000);
//	dcMax->setValueLong (20000000);
	return 0;
}

int Sitech::startPark ()
{
	if (parkPos == NULL)
	{
		return 0;
	}
	setTargetAltAz (parkPos->getAlt (), parkPos->getAz ());
	return moveAltAz ();
}

void Sitech::getConfiguration ()
{
	ra_acceleration->setValueDouble (serConn->getSiTechValue ('Y', "R"));
	dec_acceleration->setValueDouble (serConn->getSiTechValue ('X', "R"));

	ra_max_velocity->setValueDouble (motorSpeed2DegsPerSec (serConn->getSiTechValue ('Y', "S"), ra_ticks->getValueLong ()));
	dec_max_velocity->setValueDouble (motorSpeed2DegsPerSec (serConn->getSiTechValue ('X', "S"), dec_ticks->getValueLong ()));

	ra_current->setValueDouble (serConn->getSiTechValue ('Y', "C") / 100.0);
	dec_current->setValueDouble (serConn->getSiTechValue ('X', "C") / 100.0);

	ra_pwm->setValueInteger (serConn->getSiTechValue ('Y', "O"));
	dec_pwm->setValueInteger (serConn->getSiTechValue ('X', "O"));
}

int Sitech::sitechMove (int32_t ac, int32_t dc)
{
	int32_t t_ac = ac;
	int32_t t_dc = dc;

	logStream (MESSAGE_DEBUG) << "sitechMove " << r_ra_pos->getValueLong () << " " << ac << " " << r_dec_pos->getValueLong () << " " << dc << " " << partialMove->getValueInteger () << sendLog;

	// 5 deg margin in altitude and azimuth
	int ret = checkTrajectory (r_ra_pos->getValueLong (), r_dec_pos->getValueLong (), ac, dc, labs (haCpd->getValueLong () / 10), labs (decCpd->getValueLong () / 10), 1000, 5.0, 5.0, false, false);
	logStream (MESSAGE_DEBUG) << "sitechMove checkTrajectory " << r_ra_pos->getValueLong () << " " << ac << " " << r_dec_pos->getValueLong () << " " << dc << " " << ret << sendLog;
	// cannot check trajectory, log & return..
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR | MESSAGE_CRITICAL) << "cannot move to " << ac << " " << dc << sendLog;
		return -1;
	}
	// possible hit, move just to where we can go..
	if (ret > 0)
	{
		int32_t move_a = t_ac - r_ra_pos->getValueLong ();
		int32_t move_d = t_dc - r_dec_pos->getValueLong ();

		int32_t move_diff = labs (move_a) - labs (move_d);

		logStream (MESSAGE_DEBUG) << "sitechMove a " << move_a << " d " << move_d << " diff " << move_diff << sendLog;

		// move for a time, move only one axe..the one which needs more time to move
		// if we already tried DEC axis, we need to go with RA as second
		if ((move_diff > 0 && partialMove->getValueInteger () == 0) || partialMove->getValueInteger () == 2)
		{
			if (partialMove->getValueInteger () == 2)
				move_diff = labs (move_a);
			// move for a time only in RA; move_diff is positive, see check above
			if (move_a > 0)
				ac = r_ra_pos->getValueLong () + move_diff;
			else
				ac = r_ra_pos->getValueLong () - move_diff;
			dc = r_dec_pos->getValueLong ();
			ret = checkTrajectory (r_ra_pos->getValueLong (), r_dec_pos->getValueLong (), ac, dc, labs (haCpd->getValueLong () / 10), labs (decCpd->getValueLong () / 10), 1000, 5.0, 5.0, true, false);
			logStream (MESSAGE_DEBUG) << "sitechMove RA axis only " << r_ra_pos->getValueLong () << " " << ac << " " << r_dec_pos->getValueLong () << " " << dc << " " << ret << sendLog;
			if (ret == -1)
			{
				logStream (MESSAGE_ERROR | MESSAGE_CRITICAL) << "cannot move to " << ac << " " << dc << sendLog;
				return -1;
			}
			// move RA only
			ret = 1;
		}
		else
		{
			if (partialMove->getValueInteger () == 1)
				move_diff = -labs (move_d);
			// move for a time only in DEC; move_diff is negative, see check above
			ac = r_ra_pos->getValueLong ();
			if (move_d > 0)
				dc = r_dec_pos->getValueLong () - move_diff;
			else
				dc = r_dec_pos->getValueLong () + move_diff;
			ret = checkTrajectory (r_ra_pos->getValueLong (), r_dec_pos->getValueLong (), ac, dc, labs (haCpd->getValueLong () / 10), labs (decCpd->getValueLong () / 10), 1000, 5.0, 5.0, true, false);
			logStream (MESSAGE_DEBUG) << "sitechMove DEC axis only " << r_ra_pos->getValueLong () << " " << ac << " " << r_dec_pos->getValueLong () << " " << dc << " " << ret << sendLog;
			if (ret == -1)
			{
				logStream (MESSAGE_ERROR | MESSAGE_CRITICAL) << "cannot move to " << ac << " " << dc << sendLog;
				return -1;
			}
			// move DEC only
			ret = 2;
		}

		t_ra_pos->setValueLong (ac);
		t_dec_pos->setValueLong (dc);
	}

	radec_Xrequest.y_dest = ac;
	radec_Xrequest.x_dest = dc;

	radec_Xrequest.y_speed = degsPerSec2MotorSpeed (ra_speed->getValueDouble (), ra_ticks->getValueLong (), 360) * SPEED_MULTI;
	radec_Xrequest.x_speed = degsPerSec2MotorSpeed (dec_speed->getValueDouble (), dec_ticks->getValueLong (), 360) * SPEED_MULTI;

	// clear bit 4, tracking
	xbits &= ~(0x01 << 4);

	radec_Xrequest.x_bits = xbits;
	radec_Xrequest.y_bits = ybits;

	serConn->sendXAxisRequest (radec_Xrequest);

	return ret;
}

void Sitech::sitechStartTracking (bool startTimer)
{
	double sec_step = 2.0;
	// calculate position sec_step from last position, base speed on this..
	struct ln_equ_posn tarPos;
	getTarget (&tarPos);

	info ();

	int32_t ac = r_ra_pos->getValueLong ();
	int32_t dc = r_dec_pos->getValueLong ();

	int32_t homeOff;

	getHomeOffset (homeOff);

	double futureJD = getTelJD + sec_step / 86400.0;

	int ret = sky2counts (&tarPos, ac, dc, futureJD, homeOff, 0);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot calculate target position in 2 second from now, stop tracking" << sendLog;
		stopMove ();
		return;
	}

	if (use_constant_speed->getValueBool () == true)
	{
		radec_Xrequest.y_speed = fabs (ra_track_speed->getValueDouble ()) * SPEED_MULTI;
		radec_Xrequest.x_speed = fabs (dec_track_speed->getValueDouble ()) * SPEED_MULTI;

		// 2 degrees in ra; will be called periodically..
		if (ra_track_speed->getValueDouble () > 0)
			radec_Xrequest.y_dest = r_ra_pos->getValueLong () + haCpd->getValueDouble () * 2.0;
		else
			radec_Xrequest.y_dest = r_ra_pos->getValueLong () - haCpd->getValueDouble () * 2.0;

		if (dec_track_speed->getValueDouble () > 0)
			radec_Xrequest.x_dest = r_dec_pos->getValueLong () + haCpd->getValueDouble () * 2.0;
		else
			radec_Xrequest.x_dest = r_dec_pos->getValueLong () - haCpd->getValueDouble () * 2.0;
	}
	else
	{
		radec_Xrequest.y_speed = ticksPerSec2MotorSpeed (labs (ac - r_ra_pos->getValueLong ()) / sec_step);
		radec_Xrequest.x_speed = ticksPerSec2MotorSpeed (labs (dc - r_dec_pos->getValueLong ()) / sec_step);

		if (radec_Xrequest.y_speed != 0)
		{
			if (ac > r_ra_pos->getValueLong ())
				radec_Xrequest.y_dest = r_ra_pos->getValueLong () + haCpd->getValueDouble () * 2.0;
			else
				radec_Xrequest.y_dest = r_ra_pos->getValueLong () - haCpd->getValueDouble () * 2.0;
		}
		else
		{
			radec_Xrequest.y_dest = r_ra_pos->getValueLong ();
		}

		if (radec_Yrequest.x_speed != 0)
		{
			if (dc > r_dec_pos->getValueLong ())
				radec_Xrequest.x_dest = r_dec_pos->getValueLong () + haCpd->getValueDouble () * 2.0;
			else
				radec_Xrequest.x_dest = r_dec_pos->getValueLong () - haCpd->getValueDouble () * 2.0;
		}
		else
		{
			radec_Yrequest.x_dest = r_dec_pos->getValueLong ();
		}
	}

	ra_sitech_speed->setValueLong (radec_Xrequest.y_speed);
	dec_sitech_speed->setValueLong (radec_Xrequest.x_speed);

	radec_Xrequest.x_bits = xbits;
	radec_Xrequest.y_bits = ybits;

	xbits |= (0x01 << 4);

	// check that the entered trajactory is valid
	ret = checkTrajectory (ac, dc, radec_Xrequest.y_dest, radec_Xrequest.x_dest, labs (haCpd->getValueLong () / 10), labs (decCpd->getValueLong () / 10), 1000, 2.0, 2.0, false, false);
	if (ret != 0)
	{
		logStream (MESSAGE_WARNING) << "trajectory from " << ac << " " << dc << " to " << radec_Xrequest.y_dest << " " << radec_Xrequest.x_dest << " will hit (" << ret << "), stopping tracking" << sendLog;
		stopMove ();
		return;
	}

	serConn->sendXAxisRequest (radec_Xrequest);

	// make sure we will be called again
	if (startTimer && (wasStopped == false))
	{
		deleteTimers (RTS2_SITECH_TIMER);
		addTimer (TRACK_TIME_DELTA, new rts2core::Event (RTS2_SITECH_TIMER));
	}
}

double Sitech::degsPerSec2MotorSpeed (double dps, int32_t loop_ticks, double full_circle)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return ((loop_ticks / full_circle) * dps) / 1953;
		case FORCE_ONE:
			return ((loop_ticks / full_circle) * dps) / servoPIDSampleRate->getValueDouble ();
		default:
			return 0;
	}
}

double Sitech::ticksPerSec2MotorSpeed (double tps)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return tps * SPEED_MULTI / 1953;
		case FORCE_ONE:
			return tps * SPEED_MULTI / servoPIDSampleRate->getValueDouble ();
		default:
			return 0;
	}
}

double Sitech::motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return (double) speed / loop_ticks * (360.0 * 1953 / SPEED_MULTI);
		case FORCE_ONE:
			return (double) speed / loop_ticks * (360.0 * servoPIDSampleRate->getValueDouble () / SPEED_MULTI);
		default:
			return 0;
	}
}

int main (int argc, char **argv)
{	
	Sitech device = Sitech (argc, argv);
	return device.run ();
}
