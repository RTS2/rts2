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

		virtual int info ();

		virtual int startResync ();

		virtual int startMoveFixed (double tar_az, double tar_alt);

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

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

	private:
		const char *device_file;
		ConnSitech *serConn;

		SitechAxisStatus altaz_status;
		SitechYAxisRequest altaz_Yrequest;
		SitechXAxisRequest altaz_Xrequest;

		rts2core::ValueDouble *sitechVersion;
		rts2core::ValueInteger *sitechSerial;

		rts2core::ValueLong *az_pos;
		rts2core::ValueLong *alt_pos;

		rts2core::ValueLong *r_az_pos;
		rts2core::ValueLong *r_alt_pos;

		rts2core::ValueLong *t_az_pos;
		rts2core::ValueLong *t_alt_pos;

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

		rts2core::ValueInteger *countUp;
		rts2core::ValueDouble *servoPIDSampleRate;

		void getConfiguration ();

		// JD used in last getTel call
		double getTelJD;

		/**
		 * Retrieve telescope counts, convert them to RA and Declination.
		 */
		void getTel ();
		void getTel (double &telra, double &teldec, int &telflip, double &un_telra, double &un_teldec);

		void getPIDs ();
		std::string findErrors (uint16_t e);

		double motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks);

		// which controller is connected
		enum {SERVO_I, SERVO_II, FORCE_ONE} sitechType;

		uint8_t xbits;
		uint8_t ybits;
};

}

using namespace rts2teld;


SitechAltAz::SitechAltAz (int argc, char **argv):AltAz (argc,argv, true, true)
{
	device_file = "/dev/ttyUSB0";
	serConn = NULL;

	createValue (sitechVersion, "sitech_version", "SiTech controller firmware version", false);
	createValue (sitechSerial, "sitech_serial", "SiTech controller serial number", false);

	createValue (az_pos, "AXAZ", "AZ motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (alt_pos, "AXALT", "ALT motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (r_az_pos, "R_AXRA", "real AZ motor axis count", true);
	createValue (r_alt_pos, "R_AXDEC", "real ALT motor axis count", true);

	createValue (t_az_pos, "T_AXAZ", "target AZ motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (t_alt_pos, "T_AXALT", "target ALT motor axis count", true, RTS2_VALUE_WRITABLE);

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

	addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
}


SitechAltAz::~SitechAltAz(void)
{
	delete serConn;
	serConn = NULL;
}

int SitechAltAz::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
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

int SitechAltAz::info ()
{
	getTel ();
	return AltAz::info ();
}

int SitechAltAz::startResync ()
{
	return -1;
}

int SitechAltAz::startMoveFixed (double tar_az, double tar_alt)
{
	return -1;
}

int SitechAltAz::stopMove ()
{
	return -1;
}

int SitechAltAz::isMoving ()
{
	return -1;
}

int SitechAltAz::startPark()
{
	return -1;
}

void SitechAltAz::getConfiguration ()
{
	az_acceleration->setValueDouble (serConn->getSiTechValue ('Y', "R"));
	alt_acceleration->setValueDouble (serConn->getSiTechValue ('X', "R"));

	az_max_velocity->setValueDouble (motorSpeed2DegsPerSec (serConn->getSiTechValue ('Y', "S"), az_ticks->getValueLong ()));
	alt_max_velocity->setValueDouble (motorSpeed2DegsPerSec (serConn->getSiTechValue ('X', "S"), alt_ticks->getValueLong ()));

	az_current->setValueDouble (serConn->getSiTechValue ('Y', "C") / 100.0);
	alt_current->setValueDouble (serConn->getSiTechValue ('X', "C") / 100.0);

	az_pwm->setValueInteger (serConn->getSiTechValue ('Y', "O"));
	alt_pwm->setValueInteger (serConn->getSiTechValue ('X', "O"));
}

void SitechAltAz::getTel ()
{
	serConn->getAxisStatus ('X', altaz_status);

	r_az_pos->setValueLong (altaz_status.y_pos);
	r_alt_pos->setValueLong (altaz_status.x_pos);

	az_pos->setValueLong (altaz_status.y_pos + azZero->getValueDouble () * azCpd->getValueDouble ());
	alt_pos->setValueLong (altaz_status.x_pos + altZero->getValueDouble () * altCpd->getValueDouble ());

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

void SitechAltAz::getTel (double &telaz, double &telalt, int &telflip, double &un_telaz, double &un_telalt)
{
	getTel ();
}

void SitechAltAz::getPIDs ()
{
	PIDs->clear ();
	
	PIDs->addValue (serConn->getSiTechValue ('X', "PPP"));
	PIDs->addValue (serConn->getSiTechValue ('X', "III"));
	PIDs->addValue (serConn->getSiTechValue ('X', "DDD"));
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

double SitechAltAz::motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks)
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
	SitechAltAz device = SitechAltAz (argc, argv);
	return device.run ();
}
