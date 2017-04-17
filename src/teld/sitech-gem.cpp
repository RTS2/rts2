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
#include "constsitech.h"

#include "connection/sitech.h"

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

	protected:
		virtual int processOption (int in_opt);

		virtual int commandAuthorized (rts2core::Connection *conn);

		virtual int initHardware ();

		virtual int info ();

		virtual int startResync ();

		virtual int endMove ();

		virtual int stopMove ();

		virtual void telescopeAboveHorizon ();

		virtual int abortMoveTracking ();

		virtual int startPark ();

		virtual int endPark ()
		{
			return 0;
		}

		virtual int isMoving ();

		virtual int isParking ()
		{
			return isMoving ();
		}

		virtual int setTracking (int track, bool addTrackingTimer = false, bool send = true);

		/**
		 * Starts mount tracking - endless speed limited pointing.
		 * Called periodically to make sure we stay on target.
		 */
		virtual void runTracking ();

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int updateLimits ();

	private:
		void internalTracking (double sec_step, float speed_factor);

		void getConfiguration ();

		/**
		 * Sends SiTech XYS command with requested coordinates.
		 */
		int sitechMove (int32_t ac, int32_t dc);

		/**
		 * Sends SiTech XYS command with requested coordinates.
		 */
		void sitechSetTarget (int32_t ac, int32_t dc);

		/**
		 * Check if movement only in DEC axis is a possibility.
		 */
		int checkMoveDEC (double JD, int32_t &ac, int32_t &dc, int32_t move_d);

		rts2core::ConnSitech *serConn;

		rts2core::SitechAxisStatus radec_status;
		rts2core::SitechYAxisRequest radec_Yrequest;
		rts2core::SitechXAxisRequest radec_Xrequest;

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
		rts2core::ValueDouble *slowSyncDistance;
		rts2core::ValueFloat *fastSyncSpeed;
		rts2core::ValueFloat *trackingFactor;

		rts2core::IntegerArray *PIDs;

		rts2core::ValueLong *ra_enc;
		rts2core::ValueLong *dec_enc;

		rts2core::ValueInteger *extraBit;
		rts2core::ValueBool *autoModeRa;
		rts2core::ValueBool *autoModeDec;
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

		bool firstSlewCall;
		 
		/* Communications variables and routines for internal use */
		const char *device_file;

		// JD used in last getTel call
		double getTelUTC1, getTelUTC2;
		
		/**
		 * Retrieve telescope counts, convert them to RA and Declination.
		 */
		void getTel ();
		void getTel (double &telra, double &teldec, int &telflip, double &un_telra, double &un_teldec);

		/**
		 * Retrieves current PID settings
		 */
		void getPIDs ();

		uint8_t xbits;
		uint8_t ybits;

		bool wasStopped;

		int32_t lastSafeAc;
		int32_t lastSafeDc;


		time_t last_meas;
		int diff_failed_count;
		int32_t last_meas_tdiff_ac;
		int32_t last_meas_tdiff_dc;

		void recover ();

		int ra_last_errors;
		int dec_last_errors;
};

}

using namespace rts2teld;

Sitech::Sitech (int argc, char **argv):GEM (argc, argv, true, true, true, false), radec_status (), radec_Yrequest (), radec_Xrequest ()
{
	unlockPointing ();

#ifndef RTS2_LIBERFA
	setCorrections (true, true, true, true);
#endif

	ra_last_errors = 0;
	dec_last_errors = 0;

	offsetha = 0.;
	offsetdec = 0.;

	lastSafeAc = INT_MAX;
	lastSafeDc = INT_MAX;

	firstSlewCall = true;

	device_file = "/dev/ttyUSB0";

	acMargin = 0;

	wasStopped = false;

	last_meas = 0;
	diff_failed_count = 0;
	last_meas_tdiff_ac = last_meas_tdiff_dc = 0;

	createValue (sitechVersion, "sitech_version", "SiTech controller firmware version", false);
	createValue (sitechSerial, "sitech_serial", "SiTech controller serial number", false);

	createValue (ra_pos, "AXRA", "RA motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (dec_pos, "AXDEC", "DEC motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (r_ra_pos, "R_AXRA", "real RA motor axis count", true);
	createValue (r_dec_pos, "R_AXDEC", "real DEC motor axis count", true);

	createValue (t_ra_pos, "T_AXRA", "target RA motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (t_dec_pos, "T_AXDEC", "target DEC motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (partialMove, "partial_move", "1 - RA only, 2 - DEC only, 3 - DEC to pole, move just close to the horizon", false);
	partialMove->setValueInteger (0);

	createValue (trackingDist, "tracking_dist", "tracking error margin (when error drops below this value, telescope will start tracking)", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);

	// default to 1 arcsec
	trackingDist->setValueDouble (1 / 60.0 / 60.0);

	createValue (slowSyncDistance, "slow_track_distance", "distance for slow sync (at the end of movement, to catch with sky)", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);
	slowSyncDistance->setValueDouble (0.05);  // 3 arcmin

	createValue (fastSyncSpeed, "fast_sync_speed", "fast speed factor (compared to siderial trackign) for fast alignment (above slow_track_distance)", false, RTS2_VALUE_WRITABLE);
	fastSyncSpeed->setValueFloat (4);

	createValue (trackingFactor, "tracking_factor", "tracking speed multiplier", false, RTS2_VALUE_WRITABLE);
	trackingFactor->setValueFloat (0.89);

	createValue (PIDs, "pids", "axis PID values", false);

	createValue (ra_enc, "ENCRA", "RA encoder readout", true);
	createValue (dec_enc, "ENCDEC", "DEC encoder readout", true);

	createValue (extraBit, "extra_bits", "extra bits from axis status", false, RTS2_DT_HEX);
	createValue (autoModeRa, "auto_mode_ra", "RA axis auto mode", false, RTS2_DT_ONOFF);
	createValue (autoModeDec, "auto_mode_dec", "DEC axis auto mode", false, RTS2_DT_ONOFF);
	createValue (mclock, "mclock", "millisecond board clocks", false);
	createValue (temperature, "temperature", "[C] board temperature (CPU)", false);
	createValue (ra_worm_phase, "y_worm_phase", "RA worm phase", false);

	createValue (ra_last, "ra_last", "RA motor location at last RA scope encoder location change", false);
	createValue (dec_last, "dec_last", "DEC motor location at last DEC scope encoder location change", false);

	createValue (ra_errors, "ra_errors", "RA errors (only for FORCE ONE)", false);
	createValue (dec_errors, "dec_errors", "DEC errors (only for FORCE_ONE)", false);

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

	createValue (use_constant_speed, "use_constant_speed", "use precalculated speed for tracking", false, RTS2_VALUE_WRITABLE);
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

/* Full stop */
int Sitech::stopMove ()
{
	try
	{
		serConn->siTechCommand ('X', "N");
		serConn->siTechCommand ('Y', "N");

		wasStopped = true;
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "cannot stop " << er << sendLog;
		return -1;
	}
	partialMove->setValueInteger (0);
	firstSlewCall = true;
	return 0;
}

void Sitech::telescopeAboveHorizon ()
{
	lastSafeAc = r_ra_pos->getValueLong ();
	lastSafeDc = r_dec_pos->getValueLong ();
}

int Sitech::abortMoveTracking ()
{
	// recovery, ignored
	if (partialMove->getValueInteger () == 100)
		return 1;
	// check if we are close enough to last safe position..
	if (fabs (r_ra_pos->getValueLong () - lastSafeAc) < 5 * haCpd->getValueDouble () && fabs (r_dec_pos->getValueLong () - lastSafeDc) < 5 * decCpd->getValueDouble ())
	{
		logStream (MESSAGE_WARNING) << "recovering (moving to pole), last counts: " << lastSafeAc << " " << lastSafeDc << sendLog;
		recover ();
	}
	else
	{
		stopMove ();	
	}
	return 0;
}

void Sitech::getTel ()
{
	try
	{
		serConn->getAxisStatus ('X', radec_status);
	}
	catch (rts2core::Error er)
	{
		delete serConn;

		serConn = new rts2core::ConnSitech (device_file, this);
		serConn->setDebug (getDebug ());
		serConn->init ();

		serConn->flushPortIO ();
		serConn->getSiTechValue ('Y', "XY");
		return;
	}

	r_ra_pos->setValueLong (radec_status.y_pos);
	r_dec_pos->setValueLong (radec_status.x_pos);

	ra_pos->setValueLong (radec_status.y_pos + haZero->getValueDouble () * haCpd->getValueDouble ());
	dec_pos->setValueLong (radec_status.x_pos + decZero->getValueDouble () * decCpd->getValueDouble ());

	ra_enc->setValueLong (radec_status.y_enc);
	dec_enc->setValueLong (radec_status.x_enc);

	extraBit->setValueInteger (radec_status.extra_bits);
	// not stopped, not in manual mode
	autoModeRa->setValueBool ((radec_status.extra_bits & AUTO_Y) == 0);
	autoModeDec->setValueBool ((radec_status.extra_bits & AUTO_X) == 0);
	mclock->setValueLong (radec_status.mclock);
	temperature->setValueInteger (radec_status.temperature);

	ra_worm_phase->setValueInteger (radec_status.y_worm_phase);

	switch (serConn->sitechType)
	{
		case rts2core::ConnSitech::SERVO_I:
		case rts2core::ConnSitech::SERVO_II:
			ra_last->setValueLong (le32toh (*(uint32_t*) &radec_status.y_last));
			dec_last->setValueLong (le32toh (*(uint32_t*) &radec_status.x_last));
			break;
		case rts2core::ConnSitech::FORCE_ONE:
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
					ra_errors->setValueString (serConn->findErrors (ra_val));
					if (ra_last_errors != ra_val)
					{
						if (ra_val == 0)
						{
							clearHWError ();
							logStream (MESSAGE_REPORTIT | MESSAGE_INFO) << "RA axis controller error values cleared" << sendLog;
						}
						else
						{
							raiseHWError ();
							logStream (MESSAGE_ERROR) << "RA axis controller error(s): " << ra_errors->getValue () << sendLog;
						}
						ra_last_errors = ra_val;
					}
					// stop if on limits
					if ((ra_val & ERROR_LIMIT_MINUS) || (ra_val & ERROR_LIMIT_PLUS))
						stopTracking ();
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
					dec_errors->setValueString (serConn->findErrors (dec_val));
					if (dec_last_errors != dec_val)
					{
						if (dec_val == 0)
						{
							clearHWError ();
							logStream (MESSAGE_REPORTIT | MESSAGE_INFO) << "DEC axis controller error values cleared" << sendLog;
						}
						else
						{
							raiseHWError ();
							logStream (MESSAGE_ERROR) << "DEC axis controller error(s): " << dec_errors->getValue () << sendLog;
						}
						dec_last_errors = dec_val;
					}
					// stop if on limits
					if ((dec_val & ERROR_LIMIT_MINUS) || (dec_val & ERROR_LIMIT_PLUS))
						stopTracking ();
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

			ra_pos_error->setValueInteger (*(uint16_t*) &radec_status.y_last[2]);
			dec_pos_error->setValueInteger (*(uint16_t*) &radec_status.x_last[2]);
			break;
		}
	}

	if ((getState () & TEL_MASK_MOVING) == TEL_MOVING || (getState () & TEL_MASK_MOVING) == TEL_PARKING)
	{
		time_t now;
		time (&now);

		if (getStoredTargetDistance () < slowSyncDistance->getValueDouble () * 2)
		{
			valueGood (r_ra_pos);
			valueGood (r_dec_pos);
		}
		else if (last_meas + 1 < now)
		{
			int32_t diff_ac = labs (r_ra_pos->getValueLong () - t_ra_pos->getValueLong ());
			int32_t diff_dc = labs (r_dec_pos->getValueLong () - t_dec_pos->getValueLong ());
			if (last_meas > 0)
			{
				int check_failed = 0;  //0x01 - RA, 0x02 - DEC
				// check if current measurement is smaller than the last one..
				if (labs (last_meas_tdiff_ac - diff_ac) < fabs (haCpd->getValueDouble ()) * ra_speed->getValueDouble () / 10.0 && diff_ac > fabs (haCpd->getValueDouble ()) / 60.0)
				{
					valueWarning (r_ra_pos);
					check_failed |= 0x01;
				}
				else
				{
					valueGood (r_ra_pos);
				}

				if (labs (last_meas_tdiff_dc - diff_dc) < fabs (decCpd->getValueDouble ()) * dec_speed->getValueDouble () / 10.0 && diff_dc > fabs (decCpd->getValueDouble ()) / 60.0)
				{
					valueWarning (r_dec_pos);
					check_failed = 0x02;
				}
				else
				{
					valueGood (r_dec_pos);
				}

				if (check_failed != 0)
				{
					diff_failed_count++;
				}
				else
				{
					diff_failed_count = 0;
				}

				// give up after 5 failed measurements
				if (diff_failed_count > 5)
				{
					if (check_failed & 0x01)
						valueError (r_ra_pos);
					if (check_failed & 0x02)
						valueError (r_dec_pos);
					logStream (MESSAGE_ERROR) << "move does not converge, finishing" << sendLog;
					maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING, DEVICE_ERROR_HW | TEL_OBSERVING, "move does not converge");
					failedMove ();
				}
			}
			// new measurement..
			last_meas_tdiff_ac = diff_ac;
			last_meas_tdiff_dc = diff_dc;
			last_meas = now;
		}
	}

	xbits = radec_status.x_bit;
	ybits = radec_status.y_bit;
}

void Sitech::getTel (double &telra, double &teldec, int &telflip, double &un_telra, double &un_teldec)
{
#ifdef RTS2_LIBERFA
	getEraUTC (getTelUTC1, getTelUTC2);
#else
	getTelUTC1 = ln_get_julian_from_sys ();
	getTelUTC2 = 0;
#endif

	getTel ();

	int ret = counts2sky (radec_status.y_pos, radec_status.x_pos, telra, teldec, telflip, un_telra, un_teldec, getTelUTC1 + getTelUTC2);
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

void Sitech::recover ()
{
	getTel ();
	int32_t dc = getPoleTargetD (r_dec_pos->getValueLong ());
	partialMove->setValueInteger (100);
	sitechSetTarget (r_ra_pos->getValueLong (), dc);
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

	trackingInterval->setValueFloat (0.5);

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
	
	serConn = new rts2core::ConnSitech (device_file, this);
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

	if (serConn->sitechType == rts2core::ConnSitech::FORCE_ONE)
	{
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

	haCWDAngle->setValueDouble (getHACWDAngle (r_ra_pos->getValueLong ()));

	return rts2teld::GEM::infoUTC (getTelUTC1, getTelUTC2);
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
	else if (conn->isCommand ("recover"))
	{
		if (!conn->paramEnd ())
			return -2;
		recover ();
		return 0;
	}
	return GEM::commandAuthorized (conn);
}

int Sitech::startResync ()
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

	int32_t ac = r_ra_pos->getValueLong (), dc = r_dec_pos->getValueLong ();
	int ret = calculateTarget (utc1, utc2, &tar, ac, dc, true, firstSlewCall ? haSlewMargin->getValueDouble () : 0, false);
	if (ret)
		return -1;

	wasStopped = false;

	targetHaCWDAngle->setValueDouble (getHACWDAngle (ac));

	ret = sitechMove (ac, dc);
	if (ret < 0)
		return ret;

	if (firstSlewCall == true)
	{
		valueGood (r_ra_pos);
		valueGood (r_dec_pos);
		firstSlewCall = false;
	}

	partialMove->setValueInteger (ret);
	sendValueAll (partialMove);

	return 0;
}

int Sitech::isMoving ()
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

	// if resync was only partial..
	if (partialMove->getValueInteger ())
	{
		// check if we are close enough to partial target values
		if ((labs (t_ra_pos->getValueLong () - r_ra_pos->getValueLong ()) < fabs (haCpd->getValueLong ())) && (labs (t_dec_pos->getValueLong () - r_dec_pos->getValueLong ()) < fabs (decCpd->getValueLong ())))
		{
			// try new move
			startResync ();
		}
		return USEC_SEC / 10;
	}

	double tdist = getTargetDistance ();

	if (tdist > trackingDist->getValueDouble ())
	{
		// close to target, run tracking
		if (tdist < 0.5)
		{
			if (tdist < slowSyncDistance->getValueDouble ())
				internalTracking (2.0, trackingFactor->getValueFloat ());
			else
				internalTracking (2.0, fastSyncSpeed->getValueFloat ());
			return USEC_SEC * trackingInterval->getValueFloat () / 10;
		}

		// if too far away, update target
		startResync ();

		return USEC_SEC / 10;
	}

	// set ra speed to sidereal tracking
	ra_track_speed->setValueDouble (serConn->degsPerSec2MotorSpeed (15.0 / 3600.0, ra_ticks->getValueLong (), 360.0));
	sendValueAll (ra_track_speed);

	return -2;
}

int Sitech::endMove ()
{
	valueGood (r_ra_pos);
	valueGood (r_dec_pos);
	partialMove->setValueInteger (0);
	firstSlewCall = true;
	return GEM::endMove ();
}

int Sitech::setTracking (int track, bool addTrackingTimer, bool send)
{
	if (track)
	{
		wasStopped = false;
	}
	return GEM::setTracking (track, addTrackingTimer, send);
}

int Sitech::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == ra_pos)
	{
		partialMove->setValueInteger (0);
		firstSlewCall = true;
		sitechMove (newValue->getValueLong () - haZero->getValueDouble () * haCpd->getValueDouble (), dec_pos->getValueLong () - decZero->getValueDouble () * decCpd->getValueDouble ());
		return 0;
	}
	if (oldValue == dec_pos)
	{
		partialMove->setValueInteger (0);
		firstSlewCall = true;
		sitechMove (ra_pos->getValueLong () - haZero->getValueDouble () * haCpd->getValueDouble (), newValue->getValueLong () - decZero->getValueDouble () * decCpd->getValueDouble ());
		return 0;
	}

	return GEM::setValue (oldValue, newValue);
}

int Sitech::updateLimits ()
{
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

void Sitech::internalTracking (double sec_step, float speed_factor)
{
	info ();

	int32_t ac = r_ra_pos->getValueLong ();
	int32_t dc = r_dec_pos->getValueLong ();

	int32_t ac_speed = 0;
	int32_t dc_speed = 0;

	double speed_angle = 0;

	int ret = calculateTracking (getTelUTC1, getTelUTC2, sec_step, ac, dc, ac_speed, dc_speed, speed_angle);
	if (ret)
	{
		if (ret < 0)
			logStream (MESSAGE_WARNING) << "cannot calculate next tracking, aborting tracking" << sendLog;
		stopTracking ();
		return;
	}

	int32_t ac_change = 0;
	int32_t dc_change = 0;

	if (use_constant_speed->getValueBool () == true)
	{
		// constant 2 degrees tracking..
		ac_change = fabs (haCpd->getValueDouble ()) * 2.0;
		dc_change = fabs (decCpd->getValueDouble ()) * 2.0;

		radec_Xrequest.y_speed = fabs (ra_track_speed->getValueDouble ()) * SPEED_MULTI * speed_factor;
		radec_Xrequest.x_speed = fabs (dec_track_speed->getValueDouble ()) * SPEED_MULTI * speed_factor;

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
		// 1 step change
		ac_change = labs (ac - r_ra_pos->getValueLong ());
		dc_change = labs (dc - r_dec_pos->getValueLong ());

		int32_t ac_step = speed_factor * ac_change / sec_step;
		int32_t dc_step = speed_factor * dc_change / sec_step;

		radec_Xrequest.y_speed = serConn->ticksPerSec2MotorSpeed (ac_step);
		radec_Xrequest.x_speed = serConn->ticksPerSec2MotorSpeed (dc_step);

		// put axis in future

		if (radec_Xrequest.y_speed != 0)
		{
			if (ac > r_ra_pos->getValueLong ())
				radec_Xrequest.y_dest = r_ra_pos->getValueLong () + ac_step * 10;
			else
				radec_Xrequest.y_dest = r_ra_pos->getValueLong () - ac_step * 10;
		}
		else
		{
			radec_Xrequest.y_dest = r_ra_pos->getValueLong ();
		}

		if (radec_Xrequest.x_speed != 0)
		{
			if (dc > r_dec_pos->getValueLong ())
				radec_Xrequest.x_dest = r_dec_pos->getValueLong () + dc_step * 10;
			else
				radec_Xrequest.x_dest = r_dec_pos->getValueLong () - dc_step * 10;
		}
		else
		{
			radec_Xrequest.x_dest = r_dec_pos->getValueLong ();
		}
	}

	ra_sitech_speed->setValueLong (radec_Xrequest.y_speed);
	dec_sitech_speed->setValueLong (radec_Xrequest.x_speed);

	radec_Xrequest.x_bits = xbits;
	radec_Xrequest.y_bits = ybits;

	xbits |= (0x01 << 4);

	// check that the entered trajactory is valid
	ret = checkTrajectory (getTelUTC1 + getTelUTC2, r_ra_pos->getValueLong (), r_dec_pos->getValueLong (), radec_Xrequest.y_dest, radec_Xrequest.x_dest, ac_change / sec_step / 2.0, dc_change / sec_step / 2.0, TRAJECTORY_CHECK_LIMIT, 2.0, 2.0, false, false);
	if (ret == 2 && speed_factor > 1) // too big move to future, keep target
	{
		logStream (MESSAGE_INFO) << "soft stop detected while running tracking, move from " << r_ra_pos->getValueLong () << " " << r_dec_pos->getValueLong () << " only to " << radec_Xrequest.y_dest << " " << radec_Xrequest.x_dest << sendLog;
	}
	else if (ret != 0)
	{
		logStream (MESSAGE_WARNING) << "trajectory from " << r_ra_pos->getValueLong () << " " << r_dec_pos->getValueLong () << " to " << radec_Xrequest.y_dest << " " << radec_Xrequest.x_dest << " will hit (" << ret << "), stopping tracking" << sendLog;
		stopTracking ();
		return;
	}

	t_ra_pos->setValueLong (radec_Xrequest.y_dest);
	t_dec_pos->setValueLong (radec_Xrequest.x_dest);
	try
	{
		serConn->sendXAxisRequest (radec_Xrequest);
		updateTrackingFrequency ();
	}
	catch (rts2core::Error &e)
	{
		delete serConn;

		serConn = new rts2core::ConnSitech (device_file, this);
		serConn->setDebug (getDebug ());
		serConn->init ();

		serConn->flushPortIO ();
		serConn->getSiTechValue ('Y', "XY");
		serConn->sendXAxisRequest (radec_Xrequest);
		updateTrackingFrequency ();
	}
}

void Sitech::getConfiguration ()
{
	ra_acceleration->setValueDouble (serConn->getSiTechValue ('Y', "R"));
	dec_acceleration->setValueDouble (serConn->getSiTechValue ('X', "R"));

	ra_max_velocity->setValueDouble (serConn->motorSpeed2DegsPerSec (serConn->getSiTechValue ('Y', "S"), ra_ticks->getValueLong ()));
	dec_max_velocity->setValueDouble (serConn->motorSpeed2DegsPerSec (serConn->getSiTechValue ('X', "S"), dec_ticks->getValueLong ()));

	ra_current->setValueDouble (serConn->getSiTechValue ('Y', "C") / 100.0);
	dec_current->setValueDouble (serConn->getSiTechValue ('X', "C") / 100.0);

	ra_pwm->setValueInteger (serConn->getSiTechValue ('Y', "O"));
	dec_pwm->setValueInteger (serConn->getSiTechValue ('X', "O"));
}

int Sitech::sitechMove (int32_t ac, int32_t dc)
{
	logStream (MESSAGE_DEBUG) << "sitechMove " << r_ra_pos->getValueLong () << " " << ac << " " << r_dec_pos->getValueLong () << " " << dc << " " << partialMove->getValueInteger () << sendLog;

	double JD = ln_get_julian_from_sys ();

	int ret = calculateMove (JD, r_ra_pos->getValueLong (), r_dec_pos->getValueLong (), ac, dc, partialMove->getValueInteger ());
	if (ret < 0)
		return ret;

	sitechSetTarget (ac, dc);

	return ret;
}

void Sitech::sitechSetTarget (int32_t ac, int32_t dc)
{
	radec_Xrequest.y_dest = ac;
	radec_Xrequest.x_dest = dc;

	radec_Xrequest.y_speed = serConn->degsPerSec2MotorSpeed (ra_speed->getValueDouble (), ra_ticks->getValueLong (), 360) * SPEED_MULTI;
	radec_Xrequest.x_speed = serConn->degsPerSec2MotorSpeed (dec_speed->getValueDouble (), dec_ticks->getValueLong (), 360) * SPEED_MULTI;

	// clear bit 4, tracking
	xbits &= ~(0x01 << 4);

	radec_Xrequest.x_bits = xbits;
	radec_Xrequest.y_bits = ybits;

	serConn->sendXAxisRequest (radec_Xrequest);

	t_ra_pos->setValueLong (ac);
	t_dec_pos->setValueLong (dc);
}

void Sitech::runTracking ()
{
	if ((getState () & TEL_MASK_MOVING) != TEL_OBSERVING)
		return;
	internalTracking (2.0, trackingFactor->getValueFloat ());
	GEM::runTracking ();
}

int main (int argc, char **argv)
{	
	Sitech device = Sitech (argc, argv);
	return device.run ();
}
