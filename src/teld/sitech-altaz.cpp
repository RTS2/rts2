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

		virtual int isParking ();

		virtual int isOffseting ();

		virtual void runTracking ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

	private:
		void internalTracking (double sec_step, float speed_factor);

		const char *tel_tty;
		rts2core::ConnSitech *telConn;

		rts2core::SitechAxisStatus altaz_status;
		rts2core::SitechYAxisRequest altaz_Yrequest;
		rts2core::SitechXAxisRequest altaz_Xrequest;

		rts2core::ValueDouble *sitechVersion;
		rts2core::ValueInteger *sitechSerial;

		rts2core::ValueLong *az_pos;
		rts2core::ValueLong *alt_pos;

		rts2core::ValueLong *r_az_pos;
		rts2core::ValueLong *r_alt_pos;

		rts2core::ValueLong *t_az_pos;
		rts2core::ValueLong *t_alt_pos;

		rts2core::ValueDouble *trackingDist;
		rts2core::ValueFloat *trackingLook;
		rts2core::ValueDouble *slowSyncDistance;
		rts2core::ValueFloat *fastSyncSpeed;
		rts2core::ValueFloat *trackingFactor;

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

		rts2core::ValueBool *computed_only_speed;

		rts2core::ValueDouble *az_acceleration;
		rts2core::ValueDouble *alt_acceleration;

		rts2core::ValueDouble *az_max_velocity;
		rts2core::ValueDouble *alt_max_velocity;

		rts2core::ValueDouble *az_current;
		rts2core::ValueDouble *alt_current;

		rts2core::ValueInteger *az_pwm;
		rts2core::ValueInteger *alt_pwm;

		rts2core::ValueInteger *countUp;
		rts2core::ValueDouble *PIDSampleRate;

		void getConfiguration ();

		int sitechMove (int32_t azc, int32_t altc);

		void telSetTarget (int32_t ac, int32_t dc);
		void derSetTarget ();

		bool firstSlewCall;
		bool wasStopped;

		// JD used in last getTel call
		double getTelUTC1;
		double getTelUTC2;

		/**
		 * Retrieve telescope counts, convert them to RA and Declination.
		 */
		void getTel ();
		void getTel (double &telra, double &teldec, double &un_telra, double &un_telzd);

		void getPIDs ();

		uint8_t xbits;
		uint8_t ybits;

		int az_last_errors;
		int alt_last_errors;
};

}

using namespace rts2teld;

SitechAltAz::SitechAltAz (int argc, char **argv):AltAz (argc,argv, true, true, true, true, false)
{
	unlockPointing ();

#ifndef RTS2_LIBERFA
	setCorrections (true, true, true, true);
#endif

	tel_tty = "/dev/ttyUSB0";
	telConn = NULL;

	az_last_errors = 0;
	alt_last_errors = 0;

	createValue (sitechVersion, "sitech_version", "SiTech controller firmware version", false);
	createValue (sitechSerial, "sitech_serial", "SiTech controller serial number", false);

	createValue (az_pos, "AXAZ", "AZ motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (alt_pos, "AXALT", "ALT motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (r_az_pos, "R_AXAZ", "real AZ motor axis count", true);
	createValue (r_alt_pos, "R_AXALT", "real ALT motor axis count", true);

	createValue (t_az_pos, "T_AXAZ", "target AZ motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (t_alt_pos, "T_AXALT", "target ALT motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (trackingDist, "tracking_dist", "tracking error budged (bellow this value, telescope will start tracking", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);

	createValue (trackingLook, "tracking_look", "[s] future position", false, RTS2_VALUE_WRITABLE);
	trackingLook->setValueFloat (2);

	// default to 1 arcsec
	trackingDist->setValueDouble (1 / 60.0 / 60.0);

	createValue (slowSyncDistance, "slow_track_distance", "distance for slow sync (at the end of movement, to catch with sky)", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);
	slowSyncDistance->setValueDouble (0.1);  // 6 arcmin

	createValue (fastSyncSpeed, "fast_sync_speed", "fast speed factor (compared to siderial tracking) for fast alignment (above slow_track_distance)", false, RTS2_VALUE_WRITABLE);
	fastSyncSpeed->setValueFloat (4);

	createValue (trackingFactor, "tracking_factor", "tracking speed multiplier", false, RTS2_VALUE_WRITABLE);
	trackingFactor->setValueFloat (1);

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
	createValue (autoModeAz, "auto_mode_az", "AZ axis auto mode", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (autoModeAlt, "auto_mode_alt", "ALT axis auto mode", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
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

	createValue (computed_only_speed, "computed_speed", "base speed vector calculations only on calculations, do not factor current target position", false);
	computed_only_speed->setValueBool (false);

	firstSlewCall = true;
	wasStopped = false;

	addOption ('f', "telescope", 1, "telescope tty (ussualy /dev/ttyUSBx");

	addParkPosOption ();
}


SitechAltAz::~SitechAltAz(void)
{
	delete telConn;
	telConn = NULL;
}

int SitechAltAz::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			tel_tty = optarg;
			break;

		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int SitechAltAz::initHardware ()
{
	int ret;

	trackingInterval->setValueFloat (0.01);

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();

	setTelLongLat (config->getObserver ()->lng, config->getObserver ()->lat);
	setTelAltitude (config->getObservatoryAltitude ());

	/* Make the connection */
	
	/* On the Sidereal Technologies controller				*/
	/*   there is an FTDI USB to serial converter that appears as	     */
	/*   /dev/ttyUSB0 on Linux systems without other USB serial converters.   */
	/*   The serial device is known to the program that calls this procedure. */
	
	telConn = new rts2core::ConnSitech (tel_tty, this);
	telConn->setDebug (getDebug ());

	ret = telConn->init ();
	if (ret)
		return -1;
	telConn->flushPortIO ();

	xbits = telConn->getSiTechValue ('X', "B");
	ybits = telConn->getSiTechValue ('Y', "B");

	sitechVersion->setValueDouble (telConn->version);
	sitechSerial->setValueInteger (telConn->getSiTechValue ('Y', "V"));

	if (telConn->sitechType == rts2core::ConnSitech::FORCE_ONE)
	{
		createValue (countUp, "count_up", "CPU count up", false);
		countUp->setValueInteger (telConn->countUp);

		createValue (PIDSampleRate, "pid_sample_rate", "number of CPU cycles per second", false);
		PIDSampleRate->setValueDouble (telConn->PIDSampleRate);

		getPIDs ();
	}

	//SitechControllerConfiguration sconfig;
	//telConn->getConfiguration (sconfig);

	//telConn->resetController ();
	
	/* Flush the input buffer in case there is something left from startup */

	telConn->flushPortIO ();

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
		return 0;
	}
	else if (conn->isCommand ("reset_controller"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->resetController ();
		getConfiguration ();
		return 0;
	}
	else if (conn->isCommand ("go_auto"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->siTechCommand ('X', "A");
		telConn->siTechCommand ('Y', "A");
		getConfiguration ();
		return 0;
	}
	return AltAz::commandAuthorized (conn);
}

int SitechAltAz::info ()
{
	struct ln_hrz_posn hrz;
	double un_az, un_zd;
	getTel (hrz.az, hrz.alt, un_az, un_zd);

	struct ln_equ_posn pos;

	getEquFromHrz (&hrz, getTelUTC1 + getTelUTC2, &pos);
	setTelRaDec (pos.ra, pos.dec);
	setTelUnAltAz (un_zd, un_az);

	return AltAz::infoUTC (getTelUTC1, getTelUTC2);
}

int SitechAltAz::startResync ()
{
	getConfiguration ();

	double utc1, utc2;
#ifdef RTS2_LIBERFA
	getEraUTC (utc1, utc2);
#else
	utc1 = ln_get_julian_from_sys ();
	utc2 = 0;
#endif
	struct ln_equ_posn tar;

	int32_t azc = r_az_pos->getValueLong (), altc = r_alt_pos->getValueLong ();
	int ret = calculateTarget (utc1, utc2, &tar, azc, altc, true, firstSlewCall ? azSlewMargin->getValueDouble () : 0, false);

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
	try
	{
		telConn->siTechCommand ('X', "N");
		telConn->siTechCommand ('Y', "N");

		wasStopped = true;
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "cannot stop " << er << sendLog;
		return -1;
	}
	firstSlewCall = true;
	return 0;
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
			if (tdist < slowSyncDistance->getValueDouble ())
				internalTracking (trackingLook->getValueFloat (), trackingFactor->getValueFloat ());
			else
				internalTracking (2.0, fastSyncSpeed->getValueFloat ());
			return USEC_SEC * trackingInterval->getValueFloat () / 1000;
		}

		// if too far away, update target
		startResync ();

		return USEC_SEC / 1000;
	}

	return -2;
}

int SitechAltAz::isParking ()
{
	if (parkPos == NULL)
		return -1;

	info ();
	struct ln_equ_posn eq1, eq2;

	eq1.ra = telAltAz->getAz ();
	eq1.dec = telAltAz->getAlt ();
	eq2.ra = parkPos->getAz ();
	eq2.dec = parkPos->getAlt ();

	double tdist = ln_get_angular_separation (&eq1, &eq2);

	if (tdist > trackingDist->getValueDouble ())
	{
		startPark ();
		return USEC_SEC / 1000;
	}

	return -2;
}


int SitechAltAz::isOffseting ()
{
	if (wasStopped)
		return -1;

	double tdist = getTargetDistance ();
	
	if (tdist > trackingDist->getValueDouble ())
		return USEC_SEC / 1000;
	return -2;
}	

int SitechAltAz::startPark ()
{
	if (parkPos == NULL)
	{
		return 0;
	}
	setTargetAltAz (parkPos->getAlt (), parkPos->getAz ());
	wasStopped = false;
	return moveAltAz ();
}

void SitechAltAz::runTracking ()
{
	if ((getState () & TEL_MASK_MOVING) != TEL_OBSERVING)
		return;
	internalTracking (trackingLook->getValueFloat (), trackingFactor->getValueFloat ());
	AltAz::runTracking ();

	checkTracking (trackingDist->getValueDouble ());
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
	if (oldValue == autoModeAz)
	{
		telConn->siTechCommand ('Y', ((rts2core::ValueBool *) newValue)->getValueBool () ? "A" : "M0");
		return 0;
	}
	if (oldValue == autoModeAlt)
	{
		telConn->siTechCommand ('X', ((rts2core::ValueBool *) newValue)->getValueBool () ? "A" : "M0");
		return 0;
	}	

	return AltAz::setValue (oldValue, newValue);
}

void SitechAltAz::internalTracking (double sec_step, float speed_factor)
{
	info ();

	int32_t a_azc = r_az_pos->getValueLong ();
	int32_t a_altc = r_alt_pos->getValueLong ();

	int32_t azc_speed = 0;
	int32_t altc_speed = 0;

	int ret = calculateTracking (getTelUTC1, getTelUTC2, sec_step, a_azc, a_altc, azc_speed, altc_speed);
	if (ret)
	{
		if (ret < 0)
			logStream (MESSAGE_WARNING) << "cannot calculate next tracking, aborting tracking" << sendLog;
		stopTracking ();
		return;
	}

	int32_t az_change = 0;
	int32_t alt_change = 0;

	if (computed_only_speed->getValueBool () == true)
	{
		// constant 2 degrees tracking..
		az_change = fabs (azCpd->getValueDouble ()) * 2.0;
		alt_change = fabs (altCpd->getValueDouble ()) * 2.0;

		altaz_Xrequest.y_speed = fabs (az_track_speed->getValueDouble ()) * SPEED_MULTI * speed_factor;
		altaz_Xrequest.x_speed = fabs (alt_track_speed->getValueDouble ()) * SPEED_MULTI * speed_factor;

		// 2 degrees in ra; will be called periodically..
		if (az_track_speed->getValueDouble () > 0)
			altaz_Xrequest.y_dest = r_az_pos->getValueLong () + azCpd->getValueDouble () * 2.0;
		else
			altaz_Xrequest.y_dest = r_az_pos->getValueLong () - azCpd->getValueDouble () * 2.0;

		if (alt_track_speed->getValueDouble () > 0)
			altaz_Xrequest.x_dest = r_alt_pos->getValueLong () + altCpd->getValueDouble () * 2.0;
		else
			altaz_Xrequest.x_dest = r_alt_pos->getValueLong () - altCpd->getValueDouble () * 2.0;
	}
	else
	{
		az_change = labs (a_azc - r_az_pos->getValueLong ());
		alt_change = labs (a_altc - r_alt_pos->getValueLong ());

		altaz_Xrequest.y_speed = labs (telConn->ticksPerSec2MotorSpeed (azc_speed));
		altaz_Xrequest.x_speed = labs (telConn->ticksPerSec2MotorSpeed (altc_speed));

		altaz_Xrequest.y_dest = a_azc;
		altaz_Xrequest.x_dest = a_altc;
	}

	az_sitech_speed->setValueLong (altaz_Xrequest.y_speed);
	alt_sitech_speed->setValueLong (altaz_Xrequest.x_speed);

	altaz_Xrequest.x_bits = xbits;
	altaz_Xrequest.y_bits = ybits;

	xbits |= (0x01 << 4);

	// check that the entered trajactory is valid
	ret = checkTrajectory (getTelUTC1 + getTelUTC2, r_az_pos->getValueLong (), r_alt_pos->getValueLong (), altaz_Xrequest.y_dest, altaz_Xrequest.x_dest, az_change / sec_step / 2.0, alt_change / sec_step / 2.0, TRAJECTORY_CHECK_LIMIT, 2.0, 2.0, false);
	if (ret == 2 && speed_factor > 1) // too big move to future, keep target
	{
		logStream (MESSAGE_INFO) << "soft stop detected while running tracking, move from " << r_az_pos->getValueLong () << " " << r_alt_pos->getValueLong () << " only to " << altaz_Xrequest.y_dest << " " << altaz_Xrequest.x_dest << sendLog;
	}
	else if (ret != 0)
	{
		logStream (MESSAGE_WARNING) << "trajectory from " << r_az_pos->getValueLong () << " " << r_alt_pos->getValueLong () << " to " << altaz_Xrequest.y_dest << " " << altaz_Xrequest.x_dest << " will hit (" << ret << "), stopping tracking" << sendLog;
		stopTracking ();
		return;
	}

	t_az_pos->setValueLong (altaz_Xrequest.y_dest);
	t_alt_pos->setValueLong (altaz_Xrequest.x_dest);
	try
	{
		telConn->sendXAxisRequest (altaz_Xrequest);
		updateTrackingFrequency ();
	}
	catch (rts2core::Error &e)
	{
		delete telConn;

		telConn = new rts2core::ConnSitech (tel_tty, this);
		telConn->setDebug (getDebug ());
		telConn->init ();

		telConn->flushPortIO ();
		telConn->getSiTechValue ('Y', "XY");
		telConn->sendXAxisRequest (altaz_Xrequest);
		updateTrackingFrequency ();
	}
}

void SitechAltAz::getConfiguration ()
{
	az_acceleration->setValueDouble (telConn->getSiTechValue ('Y', "R"));
	alt_acceleration->setValueDouble (telConn->getSiTechValue ('X', "R"));

	az_max_velocity->setValueDouble (telConn->motorSpeed2DegsPerSec (telConn->getSiTechValue ('Y', "S"), az_ticks->getValueLong ()));
	alt_max_velocity->setValueDouble (telConn->motorSpeed2DegsPerSec (telConn->getSiTechValue ('X', "S"), alt_ticks->getValueLong ()));

	az_current->setValueDouble (telConn->getSiTechValue ('Y', "C") / 100.0);
	alt_current->setValueDouble (telConn->getSiTechValue ('X', "C") / 100.0);

	az_pwm->setValueInteger (telConn->getSiTechValue ('Y', "O"));
	alt_pwm->setValueInteger (telConn->getSiTechValue ('X', "O"));
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

	altaz_Xrequest.y_speed = telConn->degsPerSec2MotorSpeed (az_speed->getValueDouble (), az_ticks->getValueLong (), 360) * SPEED_MULTI;
	altaz_Xrequest.x_speed = telConn->degsPerSec2MotorSpeed (alt_speed->getValueDouble (), alt_ticks->getValueLong (), 360) * SPEED_MULTI;

	// clear bit 4, tracking
	xbits &= ~(0x01 << 4);

	altaz_Xrequest.x_bits = xbits;
	altaz_Xrequest.y_bits = ybits;

	telConn->sendXAxisRequest (altaz_Xrequest);

	t_az_pos->setValueLong (ac);
	t_alt_pos->setValueLong (dc);
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
	autoModeAz->setValueBool ((altaz_status.extra_bits & AUTO_Y) == 0);
	autoModeAlt->setValueBool ((altaz_status.extra_bits & AUTO_X) == 0);
	mclock->setValueLong (altaz_status.mclock);
	temperature->setValueInteger (altaz_status.temperature);

	az_worm_phase->setValueInteger (altaz_status.y_worm_phase);

	switch (telConn->sitechType)
	{
		case rts2core::ConnSitech::SERVO_I:
		case rts2core::ConnSitech::SERVO_II:
			az_last->setValueLong (le32toh (*(uint32_t*) &altaz_status.y_last));
			alt_last->setValueLong (le32toh (*(uint32_t*) &altaz_status.x_last));
			break;
		case rts2core::ConnSitech::FORCE_ONE:
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
					az_errors->setValueString (telConn->findErrors (az_val));
					if (az_last_errors != az_val)
					{
						if (az_val == 0)
						{
							clearHWError ();
							logStream (MESSAGE_REPORTIT | MESSAGE_INFO) << "azimuth axis controller error values cleared" << sendLog;
						}
						else
						{
							raiseHWError ();
							logStream (MESSAGE_ERROR) << "azimuth axis controller error(s): " << az_errors->getValue () << sendLog;
						}
						az_last_errors = az_val;
					}
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
					alt_errors->setValueString (telConn->findErrors (alt_val));
					if (alt_last_errors != alt_val)
					{
						if (alt_val == 0)
						{
							clearHWError ();
							logStream (MESSAGE_REPORTIT | MESSAGE_INFO) << "altitude axis controller error values cleared" << sendLog;
						}
						else
						{
							raiseHWError ();
							logStream (MESSAGE_ERROR) << "altitude axis controller error(s): " << alt_errors->getValue () << sendLog;
						}
						alt_last_errors = alt_val;
					}
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
#ifdef RTS2_LIBERFA
	getEraUTC (getTelUTC1, getTelUTC2);
#else
	getTelUTC1 = ln_get_julian_from_sys ();
	getTelUTC2 = 0;
#endif
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
}

int main (int argc, char **argv)
{	
	SitechAltAz device = SitechAltAz (argc, argv);
	return device.run ();
}
