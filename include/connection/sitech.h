/* 
 * Sitech axis controller connection.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONNECTION_SITECH__
#define __RTS2_CONNECTION_SITECH__

#include "serial.h"

namespace rts2core
{

/**
 * Class representing response to XXS and similar commands.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
typedef struct
{
	int8_t address;          //* Module address
	int32_t x_pos;           //* X (Alt/Dec) motor position
	int32_t y_pos;           //* Y (Az/RA) motor position
	int32_t x_enc;           //* X (Alt/Dec) encoder readout
	int32_t y_enc;           //* Y (Ax/RA) encoder readout
	// bit 17
	int8_t keypad;           //* Keypad status
	int8_t x_bit;            //* XBits
	int8_t y_bit;            //* YBits
	int8_t extra_bits;       //* Extra bits
	int16_t ain_1;           //* Analog input 1
	int16_t ain_2;           //* Analog input 2
	// bit 25
	uint32_t mclock;         //* Millisecond clock
	int8_t temperature;      //* Temperature (probably CPU)
	int8_t y_worm_phase;     //* Az/RA worm phase
	// bit 31
	uint8_t x_last[4];       //* Alt/Dec motor location at last Alt/Dec scope encoder location change
	uint8_t y_last[4];       //* Az/RA motor location at last Az/RA scope encoder location change
} SitechAxisStatus;

typedef struct
{
	int32_t x;              //* Alt/Dec motor encoder position
	int32_t y;		//* Az/RA motor encoder position
	int32_t xz;		//* Alt/Dec scope axis encoder position
	int32_t yz;		//* Az/RA scope axis encoder position
	int32_t xc;		//* Alt/Dec motor current * 100.0
	int32_t yc;		//* Az/RA motor current * 100.0
	int32_t v;		//* Controller power supply voltage * 10.0
	int32_t t;		//* Controller CPU temperature (Degs F)
	bool x_auto;		//* Alt/Dec motor mode (auto or manual)
	bool y_auto;		//* Az/RA motor mode (auto or manual)
	int32_t k;		//* Handpaddle status bits
} SitechControllerStatus;

typedef struct
{
	int32_t x_acc;		//* Alt/Dec Default Acceleration
	int32_t x_backlash;	//* Alt/Dec Backlash
	int16_t x_error_limit;	//* Alt/Dec Error Limit
	int16_t x_p_gain;	//* Alt/Dec Proportional Gain
	int16_t x_i_gain;	//* Alt/Dec Integral Gain
	int16_t x_d_gain;	//* Alt/Dec Derivative Gain
	int16_t x_o_limit;	//* Alt/Dec Output Limit, 0xFF = 100.0
	int16_t x_c_limit;	//* Alt/Dec Current Limit * 100
	int16_t x_i_limit;	//* Alt/Dec Integral Limit
	int8_t x_bits;		//* XBits

	int8_t p_0;		//* Unused / Padding??

	int32_t y_acc;		//* Azm/RA Default Acceleration
	int32_t y_backlash;	//* Azm/RA Backlash
	int16_t y_error_limit;	//* Azm/RA Error Limit
	int16_t y_p_gain;	//* Azm/RA Proportional Gain
	int16_t y_i_gain;	//* Azm/RA Integral Gain
	int16_t y_d_gain;	//* Azm/RA Derivative Gain
	int16_t y_o_limit;	//* Azm/RA Output Limit, 0xFF = 100.0
	int16_t y_c_limit;	//* Azm/RA Current Limit * 100
	int16_t y_i_limit;	//* Azm/RA Integral Limit
	int8_t y_bits;		//* YBits

	int8_t p_1;		//* Unused / Padding??

	int8_t address;		//* Address

	int8_t p_2;		//* Unused / padding??

	int32_t eq_rate;	//* Equatorial Rate
	int32_t eq_updown;	//* Equatorial UpDown adjust
	
	int32_t tracking_goal;	//* Tracking Platform Goal

	int16_t latitude;	//* Latitude * 100, MSB FIRST!!

	int32_t y_enc_ticks;	//* Azm Scope Encoder Ticks Per Rev, MSB FIRST!!
	int32_t x_enc_ticks;	//* Alt Scope Encoder Ticks Per Rev, MSB FIRST!!

	int32_t y_mot_ticks;	//* Azm Motor Ticks Per Rev, MSB FIRST!!
	int32_t x_mot_ticks;	//* Alt Motor Ticks Per Rev, MSB FIRST!!

	int32_t x_slew_rate;	//* Alt/Dec Slew Rate
	int32_t y_slew_rate;	//* Azm/RA Slew Rate

	int32_t x_pan_rate;	//* Alt/Dec Pan Rate
	int32_t y_pan_rate;	//* Azm/RA Pan Rate

	int32_t x_guide_rate;	//* Alt/Dec Guide Rate
	int32_t y_guide_rate;	//* Azm/RA Guide Rate

	int8_t pec_auto;	//* R/A PEC Auto Sync Enable (if low bit = 1), or PicServo Comm Timeout??

	int8_t p_3;		//* Unused / Padding??
	int8_t p_4;		//* Baud Rate??

	int8_t p_5;		//* Unused / Padding??

	int8_t p_6;		//* 1 = Enable Argo Navis, 2 = Enable Sky Commander

	int8_t p_7;		//* Unused / Padding??

	int32_t local_deg;	//* Local Search Degrees * 100
	int32_t local_speed;	//* Local Search Speed, arcsec per sec

	int32_t backhlash_speed;	//* Backlash speed

	int32_t y_pec_ticks;	//* RA/Azm PEC Ticks

	int8_t p_8;		//* Unused / Padding??
	int8_t p_9;		//* Unused / Padding??	 
} SitechControllerConfiguration;

/**
 * Parameters of the request (XXR) command.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SitechYAxisRequest
{
	public:
		int32_t x_dest;          //* X (Alt/Dec) motor destination, in motor counts
		int32_t x_speed;         //* X (Alt/Dec) speed (base rate), in counts per servo loop
		int32_t y_dest;          //* Y (Az/RA) motor destination, in motor counts
		int32_t y_speed;         //* Y (Az/RA) speed (base rate), in counts per servo loop
		int32_t x_rate_adder;    //* X (Alt/Dec) rate adder
		int32_t y_rate_adder;    //* Y (Az/RA) rate adder
		int32_t x_rate_adder_t;  //* X (Alt/Dec) rate adder time (in servo loops; 1953 would be 1 second)
		int32_t y_rate_adder_t;  //* Y (Az/RA) rate adder time (in servo loops; 1953 would be 1 second)
};

class SitechXAxisRequest
{
	public:
		int32_t x_dest;          //* X (Alt/Dec) motor destination, in motor counts
		int32_t x_speed;         //* X (Alt/Dec) speed (base rate), in counts per servo loop
		int32_t y_dest;          //* Y (Az/RA) motor destination, in motor counts
		int32_t y_speed;         //* Y (Az/RA) speed (base rate), in counts per servo loop
		int8_t x_bits;
		int8_t y_bits;
};

/**
 * Sitech protocol connections. Used to control Sitech mounts.
 * 
 * Protocol details should be available at:
 * https://docs.google.com/document/d/10P7L-WyDicIjkYnuNnAuRSgEJ6p6HEzD2aKb2ygj9OI/pub
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnSitech: public ConnSerial
{
	public:
		/**
		 * Construct new Sitech connection.
		 *
		 * @param devName device name
		 * @param master reference to master block
		 */
		ConnSitech (const char *devName, Block *master);

		/**
		 * Initialize connection. Switch to checksumed mode.
		 */
		virtual int init ();

		/**
		 * Switch communication to ASCI mode.
		 */
		void switchToASCI ();

		/**
		 * Switch communication to binary mode.
		 */
		void switchToBinary ();

		/**
		 * Returns error bits transcribed to string.
		 */
		std::string findErrors (uint16_t e);

		/**
		 * Reset controller errors.
		 */
		void resetErrors ();

		/**
		 * Reset SiTech controller.
		 */
		void resetController ();

		/**
		 * Execute command on the axis. Don't expect any return.
		 *
		 * @param axis Command axis (X, Y, T, U, V or W)
		 * @param cmd  Command to execute (see SiTech doc for details)
		 *
		 * @throw Error on communication error
		 */
		void siTechCommand (const char axis, const char *cmd);

		/**
		 * Return SiTech value.
		 *
		 * @param axis Axis from which value will be retrieved.
		 * @param val  Value name to retrieve
		 *
		 * @return Value of the given axis/value combination
		 *
		 * @throw Error on communication error
		 */
		int32_t getSiTechValue (const char axis, const char *val);

		/**
		 * Executes .XS request and return response.
		 *
		 * @param axis          Axis for which status will be retrieved.
		 *
		 * @throw Error on communication error
		 */
		void getAxisStatus (char axis, SitechAxisStatus &ax_status);

		/**
		 * Sends binary more request to the axis.
		 *
		 * @param axis          commanded axis
		 * @param ax_request    request to send on axis
		 */
		void sendYAxisRequest (SitechYAxisRequest &ax_request);

		void sendXAxisRequest (SitechXAxisRequest &ax_request);

		void setSiTechValue (const char axis, const char *val, int value);

		void setSiTechValueLong (const char axis, const char *val, long value);

		void setPosition (const char axis, uint32_t target, uint32_t speed);
		/**
		 * Returns SiTech controller status, including motor current.
		 */
		void getControllerStatus (SitechControllerStatus &controller_status);

		void getConfiguration (SitechControllerConfiguration &config);

		// speed conversion; see Dan manual for details
		double degsPerSec2MotorSpeed (double dps, int32_t loop_ticks, double full_circle);
		double ticksPerSec2MotorSpeed (double tps);
		double motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks);

		int version;
		int countUp;
		double PIDSampleRate;

		// which controller is connected
		enum {SERVO_I, SERVO_II, FORCE_ONE} sitechType;

		void startLogging (const char *logFileName);
		void endLogging ();

	private:
		/**
		 * Reads XXS, XXR and YXR status replies.
		 */
		void readAxisStatus (SitechAxisStatus &ax_status);

		/**
		 * Reads configuration of Sitech controller.
		 */
		void readConfiguration (SitechControllerConfiguration &config);

		void writePortChecksumed (const char *cmd, size_t len);

		uint8_t calculateChecksum (const char *buf, size_t len);

		uint16_t binaryChecksum (const char *dbuf, size_t blen, bool invertH);

		bool binary;

		int logFile;
		int logCount; // flush after n records

		/**
		 * Log sitech communication to a file.
		 */
		void logBuffer (char spec, void *data, size_t len);
};

}

#endif //!__RTS2_CONNECTION_SITECH__
