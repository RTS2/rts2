/* 
 * Sidereal Technology Controller driver for AltAz telescopes
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "altaz.h"
#include "configuration.h"
#include "constsitech.h"

#include "connection/sitech.h"

/**
 * Sitech AltAz driver.
 */
namespace rts2teld
{

/**
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SitechAltAz:public AltAz
{
	public:
		SitechAltAz (int argc, char **argv);
		~SitechAltAz (void);

	protected:
		virtual int processOption (int in_opt);

		virtual int initHardware ();

		virtual int commandAuthorized (rts2core::Connection *conn);

		virtual int info ();

		virtual int startResync ();

		virtual int moveAltAz ();

		virtual int stopMove ();

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

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

	private:
		const char *tel_tty, *der_tty;
		ConnSitech *telConn;
		ConnSitech *derConn;

		SitechAxisStatus altaz_status, der_status;
		SitechYAxisRequest altaz_Yrequest, der_Yrequest;
		SitechXAxisRequest altaz_Xrequest, der_Xrequest;

		rts2core::ValueDouble *sitechVersion;
		rts2core::ValueInteger *sitechSerial;

		rts2core::ValueLong *az_pos;
		rts2core::ValueLong *alt_pos;

		rts2core::ValueLong *r_az_pos;
		rts2core::ValueLong *r_alt_pos;

		rts2core::ValueLong *t_az_pos;
		rts2core::ValueLong *t_alt_pos;

		rts2core::ValueDouble *trackingDist;
		rts2core::ValueDouble *slowSyncDistance;

		rts2core::IntegerArray *PIDs;

		rts2core::ValueLong *az_enc;
		rts2core::ValueLong *alt_enc;

		rts2core::ValueInteger *extraBit;
		rts2core::ValueBool *autoModeAz;
		rts2core::ValueBool *autoModeAlt;
		rts2core::ValueLong *mclock;
		rts2core::ValueInteger *temperature;

		rts2core::ValueInteger *az_worm_phase;

		rts2core::ValueLong *az_last;
		rts2core::ValueLong *alt_last;

		rts2core::ValueString *az_errors;
		rts2core::ValueString *alt_errors;

		rts2core::ValueInteger *az_errors_val;
		rts2core::ValueInteger *alt_errors_val;

		rts2core::ValueInteger *az_pos_error;
		rts2core::ValueInteger *alt_pos_error;

		rts2core::ValueInteger *az_supply;
		rts2core::ValueInteger *alt_supply;

		rts2core::ValueInteger *az_temp;
		rts2core::ValueInteger *alt_temp;

		rts2core::ValueInteger *az_pid_out;
		rts2core::ValueInteger *alt_pid_out;

		// request values - speed,..
		rts2core::ValueDouble *az_speed;
		rts2core::ValueDouble *alt_speed;

		// tracking speed in controller units
		rts2core::ValueDouble *az_track_speed;
		rts2core::ValueDouble *alt_track_speed;

		rts2core::ValueLong *az_sitech_speed;
		rts2core::ValueLong *alt_sitech_speed;

		rts2core::ValueDouble *az_acceleration;
		rts2core::ValueDouble *alt_acceleration;

		rts2core::ValueDouble *az_max_velocity;
		rts2core::ValueDouble *alt_max_velocity;

		rts2core::ValueDouble *az_current;
		rts2core::ValueDouble *alt_current;

		rts2core::ValueInteger *az_pwm;
		rts2core::ValueInteger *alt_pwm;

		// derotator
		rts2core::ValueLong *r_der1_pos;
		rts2core::ValueLong *r_der2_pos;

		rts2core::ValueLong *t_der1_pos;
		rts2core::ValueLong *t_der2_pos;

		rts2core::ValueLong *der1_ticks;
		rts2core::ValueLong *der2_ticks;

		rts2core::ValueDouble *der1_acceleration;
		rts2core::ValueDouble *der2_acceleration;

		rts2core::ValueDouble *der1_max_velocity;
		rts2core::ValueDouble *der2_max_velocity;

		rts2core::ValueDouble *der1_current;
		rts2core::ValueDouble *der2_current;

		rts2core::ValueInteger *der1_pwm;
		rts2core::ValueInteger *der2_pwm;

		rts2core::ValueInteger *countUp;
		rts2core::ValueDouble *telPIDSampleRate;

		rts2core::ValueInteger *countDerUp;
		rts2core::ValueDouble *derPIDSampleRate;

		void getConfiguration ();

		int sitechMove (int32_t azc, int32_t altc);

		void telSetTarget (int32_t ac, int32_t dc);
		void derSetTarget (int32_t d1, int32_t d2);

		bool firstSlewCall;
		bool wasStopped;

		// JD used in last getTel call
		double getTelJD;

		/**
		 * Retrieve telescope counts, convert them to RA and Declination.
		 */
		void getTel ();
		void getTel (double &telra, double &teldec, double &un_telra, double &un_telzd);

		void getPIDs ();
		std::string findErrors (uint16_t e);

		// speed conversion; see Dan manual for details
		double degsPerSec2MotorSpeed (double dps, int32_t loop_ticks, double samplePID, double full_circle);
		double ticksPerSec2MotorSpeed (double tps);
		double motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks, double samplePID);

		// which controller is connected
		enum {SERVO_I, SERVO_II, FORCE_ONE} sitechType;

		uint8_t xbits;
		uint8_t ybits;

		uint8_t der_xbits;
		uint8_t der_ybits;
};

}

using namespace rts2teld;

SitechAltAz::SitechAltAz (int argc, char **argv):AltAz (argc,argv, true, true)
{
	unlockPointing ();

	tel_tty = "/dev/ttyUSB0";
	telConn = NULL;

	der_tty = NULL;
	derConn = NULL;

	createValue (sitechVersion, "sitech_version", "SiTech controller firmware version", false);
	createValue (sitechSerial, "sitech_serial", "SiTech controller serial number", false);

	createValue (az_pos, "AXAZ", "AZ motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (alt_pos, "AXALT", "ALT motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (r_az_pos, "R_AXAZ", "real AZ motor axis count", true);
	createValue (r_alt_pos, "R_AXALT", "real ALT motor axis count", true);

	createValue (t_az_pos, "T_AXAZ", "target AZ motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (t_alt_pos, "T_AXALT", "target ALT motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (trackingDist, "tracking_dist", "tracking error budged (bellow this value, telescope will start tracking", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);

	// default to 1 arcsec
	trackingDist->setValueDouble (1 / 60.0 / 60.0);

	createValue (slowSyncDistance, "slow_track_distance", "distance for slow sync (at the end of movement, to catch with sky)", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);
	slowSyncDistance->setValueDouble (0.05);  // 3 arcmin

	createValue (az_acceleration, "az_acceleration", "[deg/s^2] AZ motor acceleration", false);
	createValue (alt_acceleration, "alt_acceleration", "[deg/s^2] Alt motor acceleration", false);

	createValue (az_max_velocity, "az_max_v", "[deg/s] AZ axis maximal speed", false, RTS2_DT_DEGREES);
	createValue (alt_max_velocity, "alt_max_v", "[deg/s] ALT axis maximal speed", false, RTS2_DT_DEGREES);

	createValue (az_current, "az_current", "[A] AZ motor current", false);
	createValue (alt_current, "alt_current", "[A] ALT motor current", false);

	createValue (az_pwm, "az_pwm", "[W?] AZ motor PWM output", false);
	createValue (alt_pwm, "alt_pwm", "[W?] ALT motor PWM output", false);

	createValue (PIDs, "pids", "axis PID values", false);

	createValue (az_enc, "ENCAZ", "AZ encoder readout", true);
	createValue (alt_enc, "ENCALT", "ALT encoder readout", true);

	createValue (extraBit, "extra_bits", "extra bits from axis status", false, RTS2_DT_HEX);
	createValue (autoModeAz, "auto_mode_az", "AZ axis auto mode", false, RTS2_DT_ONOFF);
	createValue (autoModeAlt, "auto_mode_alt", "ALT axis auto mode", false, RTS2_DT_ONOFF);
	createValue (mclock, "mclock", "millisecond board clocks", false);
	createValue (temperature, "temperature", "[C] board temperature (CPU)", false);
	createValue (az_worm_phase, "y_worm_phase", "AZ worm phase", false);

	createValue (az_last, "az_last", "AZ motor location at last AZ scope encoder location change", false);
	createValue (alt_last, "alt_last", "ALT motor location at last ALT scope encoder location change", false);

	createValue (az_errors, "az_errors", "AZ errors (only for FORCE ONE)", false);
	createValue (alt_errors, "alt_errors", "ALT errors (only for FORCE_ONE)", false);

	createValue (az_errors_val, "az_errors_val", "AZ errors value", false);
	createValue (alt_errors_val, "alt_erorrs_val", "ALT errors value", false);

	createValue (az_pos_error, "az_pos_error", "AZ positional error", false);
	createValue (alt_pos_error, "alt_pos_error", "ALT positional error", false);

	createValue (az_supply, "az_supply", "[V] AZ supply", false);
	createValue (alt_supply, "alt_supply", "[V] ALT supply", false);

	createValue (az_temp, "az_temp", "[F] AZ power board CPU temperature", false);
	createValue (alt_temp, "alt_temp", "[F] ALT power board CPU temperature", false);

	createValue (az_pid_out, "az_pid_out", "AZ PID output", false);
	createValue (alt_pid_out, "alt_pid_out", "ALT PID output", false);

	createValue (az_speed, "az_speed", "[deg/s] AZ speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	createValue (alt_speed, "alt_speed", "[deg/s] ALT speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);

	createValue (az_track_speed, "az_track_speed", "RA tracking speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE);
	createValue (alt_track_speed, "alt_track_speed", "DEC tracking speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE);

	createValue (az_sitech_speed, "az_sitech_speed", "speed in controller units", false);
	createValue (alt_sitech_speed, "alt_sitech_speed", "speed in controller units", false);

	firstSlewCall = true;
	wasStopped = false;

	addOption ('f', "telescope", 1, "telescope tty (ussualy /dev/ttyUSBx");
	addOption ('F', "derotator", 1, "derotator tty (ussualy /dev/ttyUSBx");
}


SitechAltAz::~SitechAltAz(void)
{
	delete telConn;
	telConn = NULL;

	delete derConn;
	derConn = NULL;
}

int SitechAltAz::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			tel_tty = optarg;
			break;

		case 'F':
			der_tty = optarg;
			break;

		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int SitechAltAz::initHardware ()
{
	int ret;
	int numread;

	trackingInterval->setValueFloat (0.5);

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();

	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());

	/* Make the connection */
	
	/* On the Sidereal Technologies controller				*/
	/*   there is an FTDI USB to serial converter that appears as	     */
	/*   /dev/ttyUSB0 on Linux systems without other USB serial converters.   */
	/*   The serial device is known to the program that calls this procedure. */
	
	telConn = new ConnSitech (tel_tty, this);
	telConn->setDebug (getDebug ());

	ret = telConn->init ();
	if (ret)
		return -1;
	telConn->flushPortIO ();

	if (der_tty != NULL)
	{
		derConn = new ConnSitech (der_tty, this);
		derConn->setDebug (getDebug ());

		ret = derConn->init ();
		if (ret)
			return -1;
		derConn->flushPortIO ();

		createValue (r_der1_pos, "R_AXD1", "[cnts] position of first derotator", true);
		createValue (r_der2_pos, "R_AXD2", "[cnts] position of second derotator", true);

		createValue (t_der1_pos, "T_AXD1", "[cnts] target position of first derotator", true, RTS2_VALUE_WRITABLE);
		createValue (t_der2_pos, "T_AXD2", "[cnts] target position of second derotator", true, RTS2_VALUE_WRITABLE);

		createValue (der1_ticks, "der1_ticks", "[cnts] 1st derotator full circle ticks", false);
		createValue (der2_ticks, "der2_ticks", "[cnts] 1nd derotator full circle ticks", false);

		createValue (der1_acceleration, "der1_acceleration", "1st derotator", false);
		createValue (der2_acceleration, "der2_acceleration", "1st derotator", false);

		createValue (der1_max_velocity, "der1_max_velocity", "1st derotator", false);
		createValue (der2_max_velocity, "der2_max_velocity", "2nd derotator", false);

		createValue (der1_current, "der1_current", "1st derotator", false);
		createValue (der2_current, "der2_current", "2nd derotator", false);

		createValue (der1_pwm, "der1_pwm", "1st derotator", false);
		createValue (der2_pwm, "der2_pwm", "2nd derotator", false);

		der_xbits = derConn->getSiTechValue ('X', "B");
		der_ybits = derConn->getSiTechValue ('Y', "B");
	}

	xbits = telConn->getSiTechValue ('X', "B");
	ybits = telConn->getSiTechValue ('Y', "B");

	numread = telConn->getSiTechValue ('X', "V");
     
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
	sitechSerial->setValueInteger (telConn->getSiTechValue ('Y', "V"));

	if (numread < 112)
	{
		sitechType = SERVO_II;
	}
	else
	{
		sitechType = FORCE_ONE;

		createValue (countUp, "count_up", "CPU count up", false);
		countUp->setValueInteger (telConn->getSiTechValue ('Y', "XHC"));

		createValue (telPIDSampleRate, "tel_pid_sample_rate", "number of CPU cycles per second", false);
		telPIDSampleRate->setValueDouble ((CRYSTAL_FREQ / 12.0) / (SPEED_MULTI - countUp->getValueInteger ()));

		if (derConn != NULL)
		{
			createValue (countDerUp, "der_count_up", "CPU count up", false);
			countDerUp->setValueInteger (derConn->getSiTechValue ('Y', "XHC"));

			createValue (derPIDSampleRate, "der_pid_sample_rate", "number of CPU cycles per second", false);
			derPIDSampleRate->setValueDouble ((CRYSTAL_FREQ / 12.0) / (SPEED_MULTI - countDerUp->getValueInteger ()));
		}

		getPIDs ();
	}

	//SitechControllerConfiguration sconfig;
	//telConn->getConfiguration (sconfig);

	//telConn->resetController ();
	
	/* Flush the input buffer in case there is something left from startup */

	telConn->flushPortIO ();

	if (derConn)
		derConn->flushPortIO ();

	getConfiguration ();

	return 0;
}

int SitechAltAz::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("zero_motor"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->setSiTechValue ('X', "F", 0);
		telConn->setSiTechValue ('Y', "F", 0);
		return 0;
	}
	else if (conn->isCommand ("reset_errors"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->resetErrors ();
		if (derConn)
			derConn->resetErrors ();
		return 0;
	}
	else if (conn->isCommand ("reset_controller"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->resetController ();
		if (derConn)
			derConn->resetController ();
		getConfiguration ();
		return 0;
	}
	else if (conn->isCommand ("go_auto"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->siTechCommand ('X', "A");
		telConn->siTechCommand ('Y', "A");
		if (derConn)
		{
			derConn->siTechCommand ('X', "A");
			derConn->siTechCommand ('Y', "A");
		}
	
		getConfiguration ();
		return 0;
	}
/*	else if (conn->isCommand ("recover"))
	{
		if (!conn->paramEnd ())
			return -2;
		recover ();
		return 0;
	} */
	return AltAz::commandAuthorized (conn);
}

int SitechAltAz::info ()
{
	struct ln_hrz_posn hrz;
	double un_az, un_zd;
	getTel (hrz.az, hrz.alt, un_az, un_zd);

	if (derConn != NULL)
	{
		derConn->getAxisStatus ('X', der_status);

		r_der1_pos->setValueLong (der_status.y_pos);
		r_der2_pos->setValueLong (der_status.x_pos);
	}

	struct ln_equ_posn pos;

	getEquFromHrz (&hrz, getTelJD, &pos);
	setTelRaDec (pos.ra, pos.dec);
	setTelUnAltAz (un_zd, un_az);

	return AltAz::infoJD (getTelJD);
}

int SitechAltAz::startResync ()
{
	getConfiguration ();

	double JD = ln_get_julian_from_sys ();
	struct ln_equ_posn tar;

	int32_t azc = r_az_pos->getValueLong (), altc = r_alt_pos->getValueLong ();
	int ret = calculateTarget (JD, &tar, azc, altc, true, firstSlewCall ? azSlewMargin->getValueDouble () : 0);

	if (ret)
		return -1;

	wasStopped = false;

	ret = sitechMove (azc, altc);
	if (ret < 0)
		return ret;

	if (firstSlewCall == true)
	{
		valueGood (r_az_pos);
		valueGood (r_alt_pos);
		firstSlewCall = false;
	}

	return 0;

}

int SitechAltAz::moveAltAz ()
{
	struct ln_hrz_posn hrz;
	telAltAz->getAltAz (&hrz);

	int32_t taz = r_az_pos->getValueLong ();
	int32_t talt = r_alt_pos->getValueLong ();

	bool flip;

	int ret = hrz2counts (&hrz, taz, talt, 0, flip, false, 0);
	if (ret)
		return ret;

	telSetTarget (taz, talt);

	return 0;
}

int SitechAltAz::stopMove ()
{
	return -1;
}

int SitechAltAz::isMoving ()
{
	if (wasStopped)
		return -1;

	time_t now = time (NULL);
	if ((getTargetStarted () + 120) < now)
	{
		logStream (MESSAGE_ERROR) << "finished move due to timeout, target position not reached" << sendLog;
		return -1;
	}

	info ();

	double tdist = getTargetDistance ();

	if (tdist > trackingDist->getValueDouble ())
	{
		// close to target, run tracking
		if (tdist < 0.5)
		{
//			if (tdist < slowSyncDistance->getValueDouble ())
//				internalTracking (2.0, 1.0);
//			else
//				internalTracking (2.0, fastSyncSpeed->getValueFloat ());
			return USEC_SEC * trackingInterval->getValueFloat () / 10;
		}

		// if too far away, update target
		startResync ();

		return USEC_SEC / 10;
	}

	return -2;
}

int SitechAltAz::startPark()
{
	return -1;
}

int SitechAltAz::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == t_az_pos)
	{
		telSetTarget (newValue->getValueLong (), t_alt_pos->getValueLong ());
		return 0;
	}
	if (oldValue == t_alt_pos)
	{
		telSetTarget (t_az_pos->getValueLong (), newValue->getValueLong ());
		return 0;
	}

	if (oldValue == t_der1_pos)
	{
		derSetTarget (newValue->getValueLong (), t_der2_pos->getValueLong ());
		return 0;
	}

	if (oldValue == t_der2_pos)
	{
		derSetTarget (t_der1_pos->getValueLong (), newValue->getValueLong ());
		return 0;
	}

	return AltAz::setValue (oldValue, newValue);
}

void SitechAltAz::getConfiguration ()
{
	az_acceleration->setValueDouble (telConn->getSiTechValue ('Y', "R"));
	alt_acceleration->setValueDouble (telConn->getSiTechValue ('X', "R"));

	az_max_velocity->setValueDouble (motorSpeed2DegsPerSec (telConn->getSiTechValue ('Y', "S"), az_ticks->getValueLong (), telPIDSampleRate->getValueDouble ()));
	alt_max_velocity->setValueDouble (motorSpeed2DegsPerSec (telConn->getSiTechValue ('X', "S"), alt_ticks->getValueLong (), telPIDSampleRate->getValueDouble ()));

	az_current->setValueDouble (telConn->getSiTechValue ('Y', "C") / 100.0);
	alt_current->setValueDouble (telConn->getSiTechValue ('X', "C") / 100.0);

	az_pwm->setValueInteger (telConn->getSiTechValue ('Y', "O"));
	alt_pwm->setValueInteger (telConn->getSiTechValue ('X', "O"));

	if (derConn)
	{
		der1_acceleration->setValueDouble (derConn->getSiTechValue ('Y', "R"));
		der2_acceleration->setValueDouble (derConn->getSiTechValue ('X', "R"));

		der1_max_velocity->setValueDouble (motorSpeed2DegsPerSec (derConn->getSiTechValue ('Y', "S"), der1_ticks->getValueLong (), derPIDSampleRate->getValueDouble ()));
		der2_max_velocity->setValueDouble (motorSpeed2DegsPerSec (derConn->getSiTechValue ('X', "S"), der2_ticks->getValueLong (), derPIDSampleRate->getValueDouble ()));

		der1_current->setValueDouble (derConn->getSiTechValue ('Y', "C") / 100.0);
		der2_current->setValueDouble (derConn->getSiTechValue ('X', "C") / 100.0);

		der1_pwm->setValueInteger (derConn->getSiTechValue ('Y', "O"));
		der2_pwm->setValueInteger (derConn->getSiTechValue ('X', "O"));
	}
}

int SitechAltAz::sitechMove (int32_t azc, int32_t altc)
{
	logStream (MESSAGE_DEBUG) << "sitechMove " << r_az_pos->getValueLong () << " " << azc << " " << r_alt_pos->getValueLong () << " " << altc << " " << sendLog;

	double JD = ln_get_julian_from_sys ();

	int ret = calculateMove (JD, r_az_pos->getValueLong (), r_alt_pos->getValueLong (), azc, altc);
	if (ret < 0)
		return ret;

	telSetTarget (azc, altc);

	return ret;
}

void SitechAltAz::telSetTarget (int32_t ac, int32_t dc)
{
	altaz_Xrequest.y_dest = ac;
	altaz_Xrequest.x_dest = dc;

	altaz_Xrequest.y_speed = degsPerSec2MotorSpeed (az_speed->getValueDouble (), az_ticks->getValueLong (), telPIDSampleRate->getValueDouble (), 360) * SPEED_MULTI;
	altaz_Xrequest.x_speed = degsPerSec2MotorSpeed (alt_speed->getValueDouble (), alt_ticks->getValueLong (), telPIDSampleRate->getValueDouble (), 360) * SPEED_MULTI;

	// clear bit 4, tracking
	xbits &= ~(0x01 << 4);

	altaz_Xrequest.x_bits = xbits;
	altaz_Xrequest.y_bits = ybits;

	telConn->sendXAxisRequest (altaz_Xrequest);

	t_az_pos->setValueLong (ac);
	t_alt_pos->setValueLong (dc);
}

void SitechAltAz::derSetTarget (int32_t d1, int32_t d2)
{
	der_Xrequest.y_dest = d1;
	der_Xrequest.x_dest = d2;

	der_Xrequest.y_speed = degsPerSec2MotorSpeed (1, az_ticks->getValueLong (), derPIDSampleRate->getValueDouble (), 360) * SPEED_MULTI;
	der_Xrequest.x_speed = degsPerSec2MotorSpeed (1, az_ticks->getValueLong (), derPIDSampleRate->getValueDouble (), 360) * SPEED_MULTI;

	//der_xbits &= ~(0x01 << 4);
	//der_ybits &= ~(0x01 << 4);

	der_Xrequest.x_bits = der_xbits;
	der_Xrequest.y_bits = der_ybits;

	derConn->sendXAxisRequest (der_Xrequest);

	t_der1_pos->setValueLong (d1);
	t_der2_pos->setValueLong (d2);
}

void SitechAltAz::getTel ()
{
	telConn->getAxisStatus ('X', altaz_status);

	r_az_pos->setValueLong (altaz_status.y_pos);
	r_alt_pos->setValueLong (altaz_status.x_pos);

	az_pos->setValueLong (altaz_status.y_pos + azZero->getValueDouble () * azCpd->getValueDouble ());
	alt_pos->setValueLong (altaz_status.x_pos + zdZero->getValueDouble () * altCpd->getValueDouble ());

	az_enc->setValueLong (altaz_status.y_enc);
	alt_enc->setValueLong (altaz_status.x_enc);

	extraBit->setValueInteger (altaz_status.extra_bits);
	// not stopped, not in manual mode
	autoModeAz->setValueBool ((altaz_status.extra_bits & 0x20) == 0);
	autoModeAlt->setValueBool ((altaz_status.extra_bits & 0x02) == 0);
	mclock->setValueLong (altaz_status.mclock);
	temperature->setValueInteger (altaz_status.temperature);

	az_worm_phase->setValueInteger (altaz_status.y_worm_phase);

	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			az_last->setValueLong (le32toh (*(uint32_t*) &altaz_status.y_last));
			alt_last->setValueLong (le32toh (*(uint32_t*) &altaz_status.x_last));
			break;
		case FORCE_ONE:
		{
			// upper nimble
			uint16_t az_val = altaz_status.y_last[0] << 4;
			az_val += altaz_status.y_last[1];

			uint16_t alt_val = altaz_status.x_last[0] << 4;
			alt_val += altaz_status.x_last[1];

			// find all possible errors
			switch (altaz_status.y_last[0] & 0x0F)
			{
				case 0:
					az_errors_val->setValueInteger (az_val);
					az_errors->setValueString (findErrors (az_val));
					// stop if on limits
					if ((az_val & ERROR_LIMIT_MINUS) || (az_val & ERROR_LIMIT_PLUS))
						stopTracking ();
					break;

				case 1:
					az_current->setValueInteger (az_val);
					break;

				case 2:
					az_supply->setValueInteger (az_val);
					break;

				case 3:
					az_temp->setValueInteger (az_val);
					break;

				case 4:
					az_pid_out->setValueInteger (az_val);
					break;
			}


			switch (altaz_status.x_last[0] & 0x0F)
			{
				case 0:
					alt_errors_val->setValueInteger (alt_val);
					alt_errors->setValueString (findErrors (alt_val));
					// stop if on limits
					if ((alt_val & ERROR_LIMIT_MINUS) || (alt_val & ERROR_LIMIT_PLUS))
						stopTracking ();
					break;

				case 1:
					alt_current->setValueInteger (alt_val);
					break;

				case 2:
					alt_supply->setValueInteger (alt_val);
					break;

				case 3:
					alt_temp->setValueInteger (alt_val);
					break;

				case 4:
					alt_pid_out->setValueInteger (alt_val);
					break;
			}

			az_pos_error->setValueInteger (*(uint16_t*) &altaz_status.y_last[2]);
			alt_pos_error->setValueInteger (*(uint16_t*) &altaz_status.x_last[2]);
			break;
		}
	}
}

void SitechAltAz::getTel (double &telaz, double &telalt, double &un_telaz, double &un_telzd)
{
	getTelJD = ln_get_julian_from_sys ();
	getTel ();

	counts2hrz (altaz_status.y_pos, altaz_status.x_pos, telaz, telalt, un_telaz, un_telzd);
}

void SitechAltAz::getPIDs ()
{
	PIDs->clear ();
	
	PIDs->addValue (telConn->getSiTechValue ('X', "PPP"));
	PIDs->addValue (telConn->getSiTechValue ('X', "III"));
	PIDs->addValue (telConn->getSiTechValue ('X', "DDD"));

	PIDs->addValue (telConn->getSiTechValue ('Y', "PPP"));
	PIDs->addValue (telConn->getSiTechValue ('Y', "III"));
	PIDs->addValue (telConn->getSiTechValue ('Y', "DDD"));

	if (derConn)
	{
		PIDs->addValue (derConn->getSiTechValue ('X', "PPP"));
		PIDs->addValue (derConn->getSiTechValue ('X', "III"));
		PIDs->addValue (derConn->getSiTechValue ('X', "DDD"));

		PIDs->addValue (derConn->getSiTechValue ('Y', "PPP"));
		PIDs->addValue (derConn->getSiTechValue ('Y', "III"));
		PIDs->addValue (derConn->getSiTechValue ('Y', "DDD"));
	}
}

std::string SitechAltAz::findErrors (uint16_t e)
{
	std::string ret;
	if (e & ERROR_GATE_VOLTS_LOW)
		ret += "Gate Volts Low. ";
	if (e & ERROR_OVERCURRENT_HARDWARE)
		ret += "OverCurrent Hardware. ";
	if (e & ERROR_OVERCURRENT_FIRMWARE)
		ret += "OverCurrent Firmware. ";
	if (e & ERROR_MOTOR_VOLTS_LOW)
		ret += "Motor Volts Low. ";
	if (e & ERROR_POWER_BOARD_OVER_TEMP)
		ret += "Power Board Over Temp. ";
	if (e & ERROR_NEEDS_RESET)
		ret += "Needs Reset (may not have any other errors). ";
	if (e & ERROR_LIMIT_MINUS)
		ret += "Limit -. ";
	if (e & ERROR_LIMIT_PLUS)
		ret += "Limit +. ";
	if (e & ERROR_TIMED_OVER_CURRENT)
		ret += "Timed Over Current. ";
	if (e & ERROR_POSITION_ERROR)
		ret += "Position Error. ";
	if (e & ERROR_BISS_ENCODER_ERROR)
		ret += "BiSS Encoder Error. ";
	if (e & ERROR_CHECKSUM)
		ret += "Checksum Error in return from Power Board. ";
	return ret;
}

double SitechAltAz::degsPerSec2MotorSpeed (double dps, int32_t loop_ticks, double samplePID, double full_circle)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return ((loop_ticks / full_circle) * dps) / 1953;
		case FORCE_ONE:
			return ((loop_ticks / full_circle) * dps) / samplePID;
		default:
			return 0;
	}
}

double SitechAltAz::ticksPerSec2MotorSpeed (double tps)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return tps * SPEED_MULTI / 1953;
		case FORCE_ONE:
			return tps * SPEED_MULTI / telPIDSampleRate->getValueDouble ();
		default:
			return 0;
	}
}

double SitechAltAz::motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks, double samplePID)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return (double) speed / loop_ticks * (360.0 * 1953 / SPEED_MULTI);
		case FORCE_ONE:
			return (double) speed / loop_ticks * (360.0 * samplePID / SPEED_MULTI);
		default:
			return 0;
	}
}

int main (int argc, char **argv)
{	
	SitechAltAz device = SitechAltAz (argc, argv);
	return device.run ();
}
