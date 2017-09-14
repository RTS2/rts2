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
#include "expander.h"

#include "connection/sitech.h"

#define OPT_SHOW_PID	OPT_LOCAL + 7

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

		virtual void changeIdleMovingTracking ();

	private:
		void scaleTrackingLook ();
		void internalTracking (double sec_step, float speed_factor);

		void updateTelTime ();

		const char *tel_tty;
		rts2core::ConnSitech *telConn;

		rts2core::ValueString *sitechLogFile;
		rts2core::Expander *sitechLogExpander;

		rts2core::SitechYAxisRequest altaz_Yrequest;
		rts2core::SitechXAxisRequest altaz_Xrequest;

		rts2core::ValueDouble *sitechVersion;
		rts2core::ValueInteger *sitechSerial;

		rts2core::ValueLong *r_az_pos;
		rts2core::ValueLong *r_alt_pos;

		rts2core::ValueLong *t_az_pos;
		rts2core::ValueLong *t_alt_pos;

		rts2core::ValueDouble *trackingDist;
		rts2core::ValueFloat *trackingLook;
		rts2core::ValueBool *userTrackingLook;
		rts2core::ValuePID *azErrPID;
		rts2core::ValuePID *altErrPID;
		int lastTrackingNum;
		rts2core::ValueDoubleStat *speedAngle;
		rts2core::ValueDoubleStat *errorAngle;
		rts2core::ValueDouble *slowSyncDistance;
		rts2core::ValueFloat *fastSyncSpeed;
		rts2core::ValueFloat *trackingFactor;

		rts2core::ValuePID *az_curr_PID;
		rts2core::ValuePID *az_slew_PID;
		rts2core::ValuePID *az_track_PID;
		rts2core::ValuePID *alt_curr_PID;
		rts2core::ValuePID *alt_slew_PID;
		rts2core::ValuePID *alt_track_PID;

		rts2core::ValueInteger *az_integral_limit;
		rts2core::ValueInteger *alt_integral_limit;
		rts2core::ValueInteger *az_output_limit;
		rts2core::ValueInteger *alt_output_limit;
		rts2core::ValueInteger *az_current_limit;
		rts2core::ValueInteger *alt_current_limit;


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

		rts2core::ValueDoubleStat *az_pos_error;
		rts2core::ValueDoubleStat *alt_pos_error;

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

		rts2core::ValueDoubleStat *az_sitech_speed_stat;
		rts2core::ValueDoubleStat *alt_sitech_speed_stat;

#ifdef ERR_DEBUG
		rts2core::ValueDoubleStat *az_err_speed_stat;
		rts2core::ValueDoubleStat *alt_err_speed_stat;
#endif

		// osciallation prevention
		rts2core::ValueDouble *az_osc_limit;
		rts2core::ValueDouble *alt_osc_limit;

		rts2core::ValuePID *az_osc_PID;
		rts2core::ValuePID *alt_osc_PID;

		double az_osc_speed;
		double alt_osc_speed;

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

		void changeSitechLogFile ();

		uint8_t xbits;
		uint8_t ybits;

		int az_last_errors;
		int alt_last_errors;

		long last_loop;

		void setSlewPID (rts2core::ValuePID *pid, char axis);
		void setTrackPID (rts2core::ValuePID *pid, char axis);

		void correctOscillations (char axis, rts2core::ValueDoubleStat *err, rts2core::ValueDouble *limit, rts2core::ValuePID *trackPID, rts2core::ValuePID *oscPID);
		void oscOff (char axis);
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

	createValue (sitechLogFile, "sitech_logfile", "SiTech logging file", false, RTS2_VALUE_WRITABLE);
	sitechLogFile->setValueString ("");
	sitechLogExpander = NULL;

	createValue (sitechVersion, "sitech_version", "SiTech controller firmware version", false);
	createValue (sitechSerial, "sitech_serial", "SiTech controller serial number", false);

	createValue (r_az_pos, "R_AXAZ", "real AZ motor axis count", true);
	createValue (r_alt_pos, "R_AXALT", "real ALT motor axis count", true);

	createValue (t_az_pos, "T_AXAZ", "target AZ motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (t_alt_pos, "T_AXALT", "target ALT motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (trackingDist, "tracking_dist", "tracking error budget (bellow this value, telescope will start tracking", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);

	createValue (trackingLook, "tracking_look", "[s] future position", false, RTS2_VALUE_WRITABLE | RTS2_DT_TIMEINTERVAL);
	createValue (userTrackingLook, "look_user", "user specified tracking lookahead", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	trackingLook->setValueDouble (2);
	userTrackingLook->setValueBool (false);
	lastTrackingNum = 0;

	createValue (azErrPID, "az_err_PID", "PID parameters for AZ error correction", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (altErrPID, "alt_err_PID", "PID parameters for Alt error correction", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	azErrPID->setPID (0, 0.02, 0);
	altErrPID->setPID (0, 0.02, 0);

	// default to 1 arcsec
	trackingDist->setValueDouble (1 / 60.0 / 60.0);

	createValue (speedAngle, "speed_angle", "[deg] speed direction", false, RTS2_DT_DEG_DIST);
	createValue (errorAngle, "error_angle", "[deg] error direction", false, RTS2_DT_DEG_DIST);

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

	az_curr_PID = NULL;
	az_slew_PID = NULL;
	az_track_PID = NULL;
	alt_curr_PID = NULL;
	alt_slew_PID = NULL;
	alt_track_PID = NULL;

	az_integral_limit = NULL;
	alt_integral_limit = NULL;
	az_output_limit = NULL;
	alt_output_limit = NULL;
	az_current_limit = NULL;
	alt_current_limit = NULL;

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

	createValue (az_sitech_speed_stat, "az_s_speed_stat", "sitech speed statistics", false);
	createValue (alt_sitech_speed_stat, "alt_s_speed_stat", "sitech speed statistics", false);

#ifdef ERR_DEBUG
	createValue (az_err_speed_stat, "az_err_speed_stat", "sitech error speed statistics", false);
	createValue (alt_err_speed_stat, "alt_err_speed_stat", "sitech error speed statistics", false);
#endif

	createValue (az_osc_limit, "az_osc", "fraction of error / range to set osc_PID", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (alt_osc_limit, "alt_osc", "fraction of error / range to set osc_PID", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	createValue (az_osc_PID, "az_osc_PID", "PID to set when az_osc conditions occurs", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (alt_osc_PID, "alt_osc_PID", "PID to set when alt_osc conditions occurs", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	firstSlewCall = true;
	wasStopped = false;

	az_osc_speed = NAN;
	alt_osc_speed = NAN;

	last_loop = 0;

	addOption ('f', "telescope", 1, "telescope tty (ussualy /dev/ttyUSBx");
	addOption (OPT_SHOW_PID, "pid", 0, "allow PID read and edit");

	addParkPosOption ();
}


SitechAltAz::~SitechAltAz(void)
{
	delete telConn;
	telConn = NULL;

	delete sitechLogExpander;
	sitechLogExpander = NULL;
}

int SitechAltAz::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			tel_tty = optarg;
			break;

		case OPT_SHOW_PID:
			createValue (az_curr_PID, "PID_AZ", "Azimuth current PID", false);
			createValue (az_slew_PID, "PID_az_slew", "AZ slew PID", false, RTS2_VALUE_WRITABLE);
			createValue (az_track_PID, "PID_az_track", "AZ tracking PID", false, RTS2_VALUE_WRITABLE);
			createValue (alt_curr_PID, "PID_ALT", "Altitude current PID", false);
			createValue (alt_slew_PID, "PID_alt_slew", "Alt slew PID", false, RTS2_VALUE_WRITABLE);
			createValue (alt_track_PID, "PID_alt_track", "Alt tracking PID", false, RTS2_VALUE_WRITABLE);

			createValue (az_integral_limit, "az_integral_limit", "Az integral limit", false);
			createValue (alt_integral_limit, "alt_integral_limit", "Alt integral limit", false);
			createValue (az_output_limit, "az_output_limit", "Az output limit", false);
			createValue (alt_output_limit, "alt_output_limit", "Alt output limit", false);
			createValue (az_current_limit, "az_current_limit", "Az current limit", false);
			createValue (alt_current_limit, "alt_current_limit", "Alt current limit", false);

			updateMetaInformations (az_slew_PID);
			updateMetaInformations (az_track_PID);
			updateMetaInformations (alt_slew_PID);
			updateMetaInformations (alt_track_PID);

			updateMetaInformations (az_integral_limit);
			updateMetaInformations (alt_integral_limit);
			updateMetaInformations (az_output_limit);
			updateMetaInformations (alt_output_limit);
			updateMetaInformations (az_current_limit);
			updateMetaInformations (alt_current_limit);
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

	if (az_curr_PID == NULL)
		createValue (az_curr_PID, "PID_AZ", "Azimuth current PID", false);

	if (alt_curr_PID == NULL)
		createValue (alt_curr_PID, "PID_ALT", "Altitude current PID", false);

	if (telConn->sitechType == rts2core::ConnSitech::FORCE_ONE)
	{
		createValue (countUp, "count_up", "CPU count up", false);
		countUp->setValueInteger (telConn->countUp);

		createValue (PIDSampleRate, "pid_sample_rate", "number of CPU cycles per second", false);
		PIDSampleRate->setValueDouble (telConn->PIDSampleRate);

		getPIDs ();
	}

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
	else if (conn->isCommand ("pids"))
	{
		if (!conn->paramEnd ())
			return -2;
		getPIDs ();
		return 0;
	}
	else if (conn->isCommand ("flash_save"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->siTechCommand ('X', "W");
		return 0;
	}
	else if (conn->isCommand ("flash_load"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->siTechCommand ('X', "T");
		getPIDs ();
		return 0;
	}
	else if (conn->isCommand ("flash_default"))
	{
		if (!conn->paramEnd ())
			return -2;
		telConn->siTechCommand ('X', "U");
		getPIDs ();
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

	az_osc_speed = NAN;
	if (az_osc_limit->isWarning ())
		oscOff ('Y');

	alt_osc_speed = NAN;
	if (alt_osc_limit->isWarning ())
		oscOff ('X');

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

	int ret = hrz2counts (&hrz, taz, talt, 0, flip, false, 0, (getState () & TEL_PARKING) ? 1 : 0);
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
	if ((getTargetStarted () + 180) < now)
	{
		logStream (MESSAGE_ERROR) << "finished move due to timeout, target position not reached" << sendLog;
		return -1;
	}

	info ();
	double tdist = getTargetDistanceMax ();

	scaleTrackingLook ();

	trackingNum++;

	if (tdist > trackingDist->getValueDouble ())
	{
		sendPA ();
		// close to target, run tracking
		if (tdist < 0.5)
		{
			if (tdist < slowSyncDistance->getValueDouble ())
				internalTracking (trackingLook->getValueFloat (), trackingFactor->getValueFloat ());
			else
				internalTracking (15.0, fastSyncSpeed->getValueFloat ());
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
	scaleTrackingLook ();
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
	else if (oldValue == t_alt_pos)
	{
		telSetTarget (t_az_pos->getValueLong (), newValue->getValueLong ());
		return 0;
	}
	else if (oldValue == autoModeAz)
	{
		telConn->siTechCommand ('Y', dynamic_cast <rts2core::ValueBool*> (newValue)->getValueBool () ? "A" : "M0");
		return 0;
	}
	else if (oldValue == autoModeAlt)
	{
		telConn->siTechCommand ('X', dynamic_cast <rts2core::ValueBool*> (newValue)->getValueBool () ? "A" : "M0");
		return 0;
	}
	else if (oldValue == trackingLook)
	{
		userTrackingLook->setValueBool (true);
		return 0;
	}
	else if (oldValue == sitechLogFile)
	{
		if (strlen (newValue->getValue ()) > 0)
		{
			delete sitechLogExpander;
			sitechLogExpander = new rts2core::Expander ();
			changeSitechLogFile ();
		}
		else
		{
			telConn->endLogging ();
			delete sitechLogExpander;
			sitechLogExpander = NULL;
		}
		return 0;
	}
	else if (oldValue == alt_slew_PID)
	{
		rts2core::ValuePID *newP = dynamic_cast <rts2core::ValuePID*> (newValue);
		setSlewPID (newP, 'X');
	}
	else if (oldValue == alt_track_PID)
	{
		rts2core::ValuePID *newP = dynamic_cast <rts2core::ValuePID*> (newValue);
		setTrackPID (newP, 'X');
	}
	else if (oldValue == az_slew_PID)
	{
		rts2core::ValuePID *newP = dynamic_cast <rts2core::ValuePID*> (newValue);
		setSlewPID (newP, 'Y');
	}
	else if (oldValue == az_track_PID)
	{
		rts2core::ValuePID *newP = dynamic_cast <rts2core::ValuePID*> (newValue);
		setTrackPID (newP, 'Y');
	}



	return AltAz::setValue (oldValue, newValue);
}

void SitechAltAz::changeIdleMovingTracking ()
{
	changeSitechLogFile ();
}

void SitechAltAz::scaleTrackingLook ()
{
	// change tracking lookahead
	if (userTrackingLook->getValueBool () == false && abs (trackingNum - lastTrackingNum) > 3)
	{
		float change = 0;

		// scale trackingLook as needed
		if (speedAngle->getStdev () > 2)
		{
			if (trackingLook->getValueFloat () < 5)
				change = 0.1;
			else
				change = 2;
		}
		else if (isTracking () && trackingLook->getValueFloat () < 10 && az_sitech_speed_stat->getMax () < 10000 && alt_sitech_speed_stat->getMax () < 10000)
		{
			trackingLook->setValueFloat (10);
		}
		else if (trackingLook->getValueFloat () < 2.5 && az_sitech_speed_stat->getMax () < 10000 && alt_sitech_speed_stat->getMax () < 10000)
		{
			trackingLook->setValueFloat (2.5);
		}
		else if (trackingLook->getValueFloat () > 1.5)
		{
			if (trackingLook->getValueFloat () > 5)
				change = -2;
			else
				change = -0.1;
		}

		if (change != 0)
		{
			trackingLook->setValueFloat (trackingLook->getValueFloat () + change);
			if (trackingLook->getValueFloat () > 15)
				trackingLook->setValueFloat (15);
			else if (trackingLook->getValueFloat () < 2.5 && az_sitech_speed_stat->getMax () < 10000 && alt_sitech_speed_stat->getMax () < 10000)
				trackingLook->setValueFloat (2.5);
			else if (trackingLook->getValueFloat () < 1.5)
				trackingLook->setValueFloat (1.5);
		}
		sendValueAll (trackingLook);
		lastTrackingNum = trackingNum;
	}
}


void SitechAltAz::internalTracking (double sec_step, float speed_factor)
{
	info ();

	int32_t a_azc = r_az_pos->getValueLong ();
	int32_t a_altc = r_alt_pos->getValueLong ();

	int32_t old_azc = a_azc;
	int32_t old_altc = a_altc;

	double azc_speed = 0;
	double altc_speed = 0;

	double aze_speed = 0;
	double alte_speed = 0;

	double speed_angle = 0;
	double error_angle = 0;

	int ret = calculateTracking (getTelUTC1, getTelUTC2, sec_step, a_azc, a_altc, azc_speed, altc_speed, aze_speed, alte_speed, speed_angle, error_angle);
	if (ret)
	{
		if (ret < 0)
			logStream (MESSAGE_WARNING) << "cannot calculate next tracking, aborting tracking" << sendLog;
		stopTracking ();
		return;
	}

	double az_change = 0;
	double alt_change = 0;

	az_change = (a_azc - r_az_pos->getValueLong ()) * 300;
	alt_change = (a_altc - r_alt_pos->getValueLong ()) * 300;

	//std::cout << std::fixed << "aa " << a_azc << " " << az_change << " " << a_altc << " " << alt_change << " " << az_change << " " << alt_change << " " << azc_speed << " " << altc_speed << " " << aze_speed << " " << alte_speed << std::endl;

	a_azc += az_change;
	a_altc += alt_change;

	double loop_sec = (mclock->getValueLong () - last_loop) / 1000.0;

	if (loop_sec < 1)
	{
		if (abs (aze_speed) > 3 || !isTracking ())
		{
			double err_sp = azErrPID->loop (aze_speed, loop_sec);
			if (getTargetDistanceMax () < trackingDist->getValueDouble ())
			{
				double err_cap = abs(azc_speed * 0.1);
				if (abs(azc_speed) < 10)
					err_cap = 10;

				if (err_sp > err_cap)
					err_sp = err_cap;
				else if (err_sp < -err_cap)
					err_sp = -err_cap;
			}

			// properly handles sign change
			azc_speed += err_sp;
#ifdef ERR_DEBUG
			az_err_speed_stat->addValue (err_sp, 20);
			az_err_speed_stat->calculate ();
			sendValueAll (az_err_speed_stat);
#endif

			if (azc_speed > 0)
			{
				a_azc = old_azc + az_change;
				a_altc = old_altc + alt_change;
			}
			else
			{
				a_azc = old_azc + az_change;
				a_altc = old_altc + alt_change;
			}
		}

		if (abs (alte_speed) > 3 || !isTracking ())
		{
			double err_sp = altErrPID->loop (alte_speed, loop_sec);
			if (getTargetDistanceMax () < trackingDist->getValueDouble ())
			{
				double err_cap = abs(altc_speed * 0.1);
				if (abs(altc_speed) < 10)
					err_cap = 10;

				if (err_sp > err_cap)
					err_sp = err_cap;
				else if (err_sp < -err_cap)
					err_sp = -err_cap;
			}

			altc_speed += err_sp;
#ifdef ERR_DEBUG
			alt_err_speed_stat->addValue (err_sp, 20);
			alt_err_speed_stat->calculate ();
			sendValueAll (alt_err_speed_stat);
#endif

			if (altc_speed > 0)
			{
				a_azc = old_azc + az_change;
				a_altc = old_altc + alt_change;
			}
			else
			{
				a_azc = old_azc + az_change;
				a_altc = old_altc + alt_change;
			}
		}
	}

	last_loop = mclock->getValueLong ();

	try
	{
		if (telConn == NULL)
		{
			telConn = new rts2core::ConnSitech (tel_tty, this);
			telConn->setDebug (getDebug ());
			telConn->init ();

			telConn->flushPortIO ();
			telConn->getSiTechValue ('Y', "XY");
			getConfiguration ();
		}
	}
	catch (rts2core::Error &e)
	{
		delete telConn;
		telConn = NULL;

		logStream (MESSAGE_ERROR) << "cannot open connection to serial port" << sendLog;
		usleep (USEC_SEC / 15);
	}

	if (a_azc > azMax->getValueLong ())
		a_azc = azMax->getValueLong ();
	if (a_azc < azMin->getValueLong ())
		a_azc = azMin->getValueLong ();

	if (a_altc > altMax->getValueLong ())
		a_altc = altMax->getValueLong ();
	if (a_altc < altMin->getValueLong ())
		a_altc = altMin->getValueLong ();

	altaz_Xrequest.y_speed = labs (telConn->ticksPerSec2MotorSpeed (azc_speed * speed_factor));
	altaz_Xrequest.x_speed = labs (telConn->ticksPerSec2MotorSpeed (altc_speed * speed_factor));

	altaz_Xrequest.y_dest = a_azc;
	altaz_Xrequest.x_dest = a_altc;

	az_sitech_speed->setValueLong (altaz_Xrequest.y_speed);
	alt_sitech_speed->setValueLong (altaz_Xrequest.x_speed);

	az_sitech_speed_stat->addValue (altaz_Xrequest.y_speed, 20);
	alt_sitech_speed_stat->addValue (altaz_Xrequest.x_speed, 20);

	az_sitech_speed_stat->calculate ();
	alt_sitech_speed_stat->calculate ();

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

	speedAngle->addValue (speed_angle, 15);
	errorAngle->addValue (error_angle, 15);

	speedAngle->calculate ();
	errorAngle->calculate ();

	try
	{
		telConn->sendXAxisRequest (altaz_Xrequest);
		updateTelTime ();
		updateTrackingFrequency ();
	}
	catch (rts2core::Error &e)
	{
		delete telConn;
		telConn = NULL;

		logStream (MESSAGE_ERROR) << "closign connection to serial port" << sendLog;
		usleep (USEC_SEC / 15);
	}
}

void SitechAltAz::updateTelTime ()
{
#ifdef RTS2_LIBERFA
	getEraUTC (getTelUTC1, getTelUTC2);
#else
	getTelUTC1 = ln_get_julian_from_sys ();
	getTelUTC2 = 0;
#endif
}

void SitechAltAz::getConfiguration ()
{
	if (telConn == NULL)
		return;
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

	altaz_Xrequest.y_speed = labs (telConn->degsPerSec2MotorSpeed (az_speed->getValueDouble (), az_ticks->getValueLong (), 360) * SPEED_MULTI);
	altaz_Xrequest.x_speed = labs (telConn->degsPerSec2MotorSpeed (alt_speed->getValueDouble (), alt_ticks->getValueLong (), 360) * SPEED_MULTI);

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
	// update data only if telescope is not tracking - if it is tracking, commands to set target will return last_status
	if (!isTracking ())
	{
		try
		{
			if (telConn == NULL)
			{
				telConn = new rts2core::ConnSitech (tel_tty, this);
				telConn->setDebug (getDebug ());
				telConn->init ();

				telConn->flushPortIO ();
				telConn->getSiTechValue ('Y', "XY");
				getConfiguration ();
			}
			telConn->getAxisStatus ('X');
			updateTelTime ();
		} catch (rts2core::Error &e)
		{
			delete telConn;
			telConn = NULL;

			logStream (MESSAGE_ERROR) << "closign connection to serial port" << sendLog;
			usleep (USEC_SEC / 15);
			return;
		}
	}
	else
	{
		if (telConn == NULL)
			return;
	}

	az_enc->setValueLong (telConn->last_status.y_enc);
	alt_enc->setValueLong (telConn->last_status.x_enc);

	extraBit->setValueInteger (telConn->last_status.extra_bits);
	// not stopped, not in manual mode
	autoModeAz->setValueBool ((telConn->last_status.extra_bits & AUTO_Y) == 0);
	autoModeAlt->setValueBool ((telConn->last_status.extra_bits & AUTO_X) == 0);
	mclock->setValueLong (telConn->last_status.mclock);
	temperature->setValueInteger (telConn->last_status.temperature);

	az_worm_phase->setValueInteger (telConn->last_status.y_worm_phase);

	switch (telConn->sitechType)
	{
		case rts2core::ConnSitech::SERVO_I:
		case rts2core::ConnSitech::SERVO_II:
			r_az_pos->setValueLong (telConn->last_status.y_pos);
			r_alt_pos->setValueLong (telConn->last_status.x_pos);

			az_last->setValueLong (le32toh (*(uint32_t*) &(telConn->last_status.y_last)));
			alt_last->setValueLong (le32toh (*(uint32_t*) &(telConn->last_status.x_last)));
			break;
		case rts2core::ConnSitech::FORCE_ONE:
		{
			// upper nimble
			uint16_t az_val = telConn->last_status.y_last[0] << 4;
			az_val += telConn->last_status.y_last[1];

			uint16_t alt_val = telConn->last_status.x_last[0] << 4;
			alt_val += telConn->last_status.x_last[1];

			// find all possible errors
			switch (telConn->last_status.y_last[0] & 0x0F)
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


			switch (telConn->last_status.x_last[0] & 0x0F)
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

			int16_t az_err = *(int16_t*) &(telConn->last_status.y_last[2]);
			int16_t alt_err = *(int16_t*) &(telConn->last_status.x_last[2]);

			if (abs (az_err) <= 7)
				r_az_pos->setValueLong (telConn->last_status.y_pos);
			else
				r_az_pos->setValueLong (telConn->last_status.y_pos - az_err);

			if (abs (alt_err) <= 7)
				r_alt_pos->setValueLong (telConn->last_status.x_pos);
			else
				r_alt_pos->setValueLong (telConn->last_status.x_pos - alt_err);

			az_pos_error->addValue (az_err, 20);
			alt_pos_error->addValue (alt_err, 20);

			correctOscillations ('Y', az_pos_error, az_osc_limit, az_track_PID, az_osc_PID);
			correctOscillations ('X', alt_pos_error, alt_osc_limit, alt_track_PID, alt_osc_PID);

			if (az_pos_error->getRange () > 50 || abs (az_pos_error->getMin ()) > 50 || abs (az_pos_error->getMax ()) > 50)
				valueWarning (az_pos_error);
			else
				valueGood (az_pos_error);

			if (alt_pos_error->getRange () > 50 || abs (alt_pos_error->getMin ()) > 50 || abs (alt_pos_error->getMax ()) > 50)
				valueWarning (alt_pos_error);
			else
				valueGood (alt_pos_error);

			r_az_pos->setValueLong (telConn->last_status.y_pos);
			r_alt_pos->setValueLong (telConn->last_status.x_pos);

			az_pos_error->calculate ();
			alt_pos_error->calculate ();
			break;
		}
	}
}

void SitechAltAz::getTel (double &telaz, double &telalt, double &un_telaz, double &un_telzd)
{
	getTel ();
	if (telConn == NULL)
		return;

	counts2hrz (telConn->last_status.y_pos, telConn->last_status.x_pos, telaz, telalt, un_telaz, un_telzd);
}

void SitechAltAz::getPIDs ()
{
	telConn->flashLoad ();

	alt_curr_PID->setPID (telConn->getSiTechValue ('X', "PPP"), telConn->getSiTechValue ('X', "III"), telConn->getSiTechValue ('X', "DDD"));
	if (alt_slew_PID)
	{
		alt_slew_PID->setPID (telConn->getSiTechValue ('X', "PP"), telConn->getSiTechValue ('X', "II"), telConn->getSiTechValue ('X', "DD"));
	}
	if (alt_track_PID)
	{
		if (!alt_osc_limit->isWarning ())
			alt_track_PID->setPID (telConn->getSiTechValue ('X', "P"), telConn->getSiTechValue ('X', "I"), telConn->getSiTechValue ('X', "D"));
	}

	az_curr_PID->setPID (telConn->getSiTechValue ('Y', "PPP"), telConn->getSiTechValue ('Y', "III"), telConn->getSiTechValue ('Y', "DDD"));
	if (az_slew_PID)
	{
		az_slew_PID->setPID (telConn->getSiTechValue ('Y', "PP"), telConn->getSiTechValue ('Y', "II"), telConn->getSiTechValue ('Y', "DD"));
	}
	if (az_track_PID)
	{
		if (!az_osc_limit->isWarning ())
			az_track_PID->setPID (telConn->getSiTechValue ('Y', "P"), telConn->getSiTechValue ('Y', "I"), telConn->getSiTechValue ('Y', "D"));
	}

	if (alt_integral_limit)
	{
		alt_integral_limit->setValueInteger (telConn->getFlashInt16 (24));
	}

	if (az_integral_limit)
	{
		az_integral_limit->setValueInteger (telConn->getFlashInt16 (124));
	}

	if (alt_output_limit)
	{
		alt_output_limit->setValueInteger (telConn->getFlashInt16 (18));
	}

	if (az_output_limit)
	{
		az_output_limit->setValueInteger (telConn->getFlashInt16 (118));
	}

	if (alt_current_limit)
	{
		alt_current_limit->setValueInteger (telConn->getFlashInt16 (20));
	}

	if (az_current_limit)
	{
		az_current_limit->setValueInteger (telConn->getFlashInt16 (120));
	}
}

void SitechAltAz::changeSitechLogFile ()
{
	try
	{
		if (sitechLogExpander)
		{
			sitechLogExpander->setExpandDate ();
			telConn->startLogging (sitechLogExpander->expandPath (sitechLogFile->getValue ()).c_str ());
		}
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_WARNING) << "cannot expand " << sitechLogFile->getValue () << sendLog;
	}
}

void SitechAltAz::setSlewPID (rts2core::ValuePID *pid, char axis)
{
	telConn->setSiTechValue (axis, "PP", pid->getP ());
	telConn->setSiTechValue (axis, "II", pid->getI ());
	telConn->setSiTechValue (axis, "DD", pid->getD ());
}

void SitechAltAz::setTrackPID (rts2core::ValuePID *pid, char axis)
{
	telConn->setSiTechValue (axis, "P", pid->getP ());
	telConn->setSiTechValue (axis, "I", pid->getI ());
	telConn->setSiTechValue (axis, "D", pid->getD ());
}

void SitechAltAz::correctOscillations (char axis, rts2core::ValueDoubleStat *err, rts2core::ValueDouble *limit, rts2core::ValuePID *trackPID, rts2core::ValuePID *oscPID)
{
	if (isnan (limit->getValueDouble ()))
		return;

	if (err->getMin () < 0 && err->getMax () > 0 && (abs (err->getValueDouble () / err->getRange ()) < limit->getValueDouble ()))
	{
		if (!limit->isWarning ())
		{
			logStream (MESSAGE_INFO) << (axis == 'X' ? "Alt" : "Az") << " setting oscillation PIDs" << sendLog;
			setTrackPID (oscPID, axis);
			valueWarning (limit);
			if (axis == 'X')
				alt_osc_speed = alt_sitech_speed_stat->getValueDouble ();
			else
				az_osc_speed = az_sitech_speed_stat->getValueDouble ();
		}
	}
	else
	{
		if (limit->isWarning ())
		{
			// wait for speed to drop before changing PID
			if (axis == 'X')
			{
				if (!isnan (alt_osc_speed) && alt_sitech_speed_stat->getValueDouble () > alt_osc_speed * 0.7)
					return;
				alt_osc_speed = NAN;
			}
			else
			{
				if (!isnan (az_osc_speed) && az_sitech_speed_stat->getValueDouble () > az_osc_speed * 0.7)
					return;
				az_osc_speed = NAN;
			}

			logStream (MESSAGE_INFO) << (axis == 'X' ? "Alt" : "Az") << " setting normal tracking PIDs" << sendLog;
			setTrackPID (trackPID, axis);
			valueGood (limit);
		}
	}
}

void SitechAltAz::oscOff (char axis)
{
	logStream (MESSAGE_INFO) << (axis == 'X' ? "Alt" : "Az") << " setting normal tracking PIDs" << sendLog;
	setTrackPID ((axis == 'X' ? alt_track_PID : az_track_PID), axis);
	valueGood (axis == 'X' ? alt_osc_limit : az_osc_limit);
}

int main (int argc, char **argv)
{	
	SitechAltAz device = SitechAltAz (argc, argv);
	return device.run ();
}
