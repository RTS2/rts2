/* 
 * RS-232 TGDrives interface.
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

#include "serial.h"
#include "device.h"

// interesting addresses

/** TGA status. */
#define TGA_STATUS                       0x01A0

/** TGA falts. */
#define TGA_FAULTS                       0x01A1

/** Servo master command. */
#define TGA_MASTER_CMD                   0x01ED

/** Servo control mode. */
#define TGA_MODE                         0x01EF

/** Digital torque mode */
#define TGA_MODE_DT                      0x1001

/** Digital speed mode */
#define TGA_MODE_DS                      0x2002

/** Analog torque mode */
#define TGA_MODE_AT                      0x8001

/** Analog speed mode */
#define TGA_MODE_AS                     0x10001

/** Stepper motor mode */
#define TGA_MODE_SM                     0x20004

/** Position absolute mode */
#define TGA_MODE_PA                     0x4004

/** Position relative (PG) */
#define TGA_MODE_PR_PG                 0x40004

/** Position relative (resolver) */
#define TGA_MODE_PR_RS                 0x80004

/** Encoder mode */
#define TGA_MODE_ENCODER              0x100004

/** Trajecory CAN mode */
#define TGA_MODE_CAN                  0x200004

/** Setting after reset */
#define TGA_AFTER_RESET                 0x01EE

/** After reset disabled */
#define TGA_AFTER_RESET_DISABLED        2

/** After reset enabled */
#define TGA_AFTER_RESET_ENABLED         1

/** Desired current */
#define TGA_DESCUR                      0x0190

/** Actual current */
#define TGA_ACTCUR                      0x01A8

/** Target position */
#define TGA_TARPOS                      0x0192

/** Current positon */
#define TGA_CURRPOS                     0x0196

/** Position error */
#define TGA_POSERR                      0x0287

/** maximal position error */
#define TGA_MAXPOSERR                   0x01CB

/** Desired speed */
#define TGA_DSPEED                      0x0194

/** Actual speed */
#define TGA_ASPEED                      0x019A

/** Maximal speed */
#define TGA_VMAX                        0x01D2

/** Acceleration */
#define TGA_ACCEL                       0x01D4

/** Deceleration */
#define TGA_DECEL                       0x01D6

#define TGA_EMERDECEL                   0x0204 

/** Position regulator parameters */
#define TGA_POSITIONREG_KP              0x01E7
 
/** Speed regulator parameters */
#define TGA_SPEEDREG_KP                 0x01D8
 
#define TGA_SPEEDREG_KI                 0x01D9 

#define TGA_SPEEDREG_IMAX               0x01DC 
 
/** Current q-component (momentum) regulator parameters */
#define TGA_Q_CURRENTREG_KP             0x01DD
 
#define TGA_Q_CURRENTREG_KI             0x01DE 

/** Current d-component (excitation) regulator parameters */
#define TGA_D_CURRENTREG_KP             0x01E2
 
#define TGA_D_CURRENTREG_KI             0x01E3 

/** Firmware version */
#define TGA_FIRMWARE                    0x01B0

// unit conversion factors

// convert speed to inc/sec
#define TGA_SPEEDFACTOR                 (262.144 * 65536.0)

// convert current to amps
#define TGA_CURRENTFACTOR               (1017.6 * 1.41)

#define TGA_ACCELFACTOR                 178957.0

#define TGA_GAINFACTOR                  327.67

namespace rts2teld
{

/**
 * Error reported by TGDrive.
 */
class TGDriveError: public rts2core::Error
{
	public:
		TGDriveError (char _status)
		{
			status = _status;
		}
	private:
		char status;
};

/**
 * Class for RS-232 connection to TGDrives motors. See http://www.tgdrives.cz fro details.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TGDrive: public rts2core::ConnSerial
{
	public:
		/**
		 * Create connection to TGDrive motor.
		 *
		 * @param _devName  Device name (ussually /dev/ttySx)
		 * @param prefix    Device prefix (usually RA or DEC)
		 * @param _master   Controlling block
		 */
		TGDrive (const char *_devName, const char *prefix, rts2core::Device *_master);

		virtual int init ();

		/**
		 * get info from unit, write it to value.
		 *
		 * @throw TGDriveError on error
		 */
		void info ();


		/**
		 * Set values.
		 *
		 *
		 * @return 1 if value does not belong to this unit
		 *
		 * @see rts2core::Block::setValue()
		 */
		int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		void setMode (int32_t mode);

		void setTargetPos (int32_t pos);
		void setCurrentPos (int32_t pos);

		void setTargetSpeed (int32_t dspeed, bool changeMode = true);
		void setMaxSpeed (double speed);


		int32_t getPosition () { return aPos->getValueInteger (); }

		bool isInSpeedMode () { return tgaMode->getValueInteger () == TGA_MODE_DS; }
		bool isInPositionMode () { return tgaMode->getValueInteger () == TGA_MODE_PA; }
		bool isInStepperMode () { return tgaMode->getValueInteger () == TGA_MODE_SM; }

		bool isMoving () { return (appStatus->getValueInteger () & 0x0a) == 0x00; }
		bool isMovingSpeed () { return (appStatus->getValueInteger () & 0x08) == 0x00; }
		bool isMovingPosition () { return (appStatus->getValueInteger () & 0x02) == 0x00; }

		void stop ();

		bool checkStop ();
		bool checkStopPlain ();

		void reset ();

		/**
		 * Read word (2 bytes) from interface.
		 *
		 * @param address Address.
		 * @return Read data.
		 *
		 * @throw TGDriveError on error.
		 */
		int16_t read2b (int16_t address);

		/**
		 * Write word (2 bytes) to interface.
		 *
		 * @param address Address.
		 * @return data Data.
		 *
		 * @throw TGDriveError on error.
		 */
		void write2b (int16_t address, int16_t data);

		/**
		 * Read long (4 bytes) from interface.
		 *
		 * @param address Address.
		 * @return Read data.
		 *
		 * @throw TGDriveError on error.
		 */
		int32_t read4b (int16_t address);

		/**
		 * Write word (2 bytes) to interface.
		 *
		 * @param address Address.
		 * @return data Data.
		 *
		 * @throw TGDriveError on error.
		 */
		void write4b (int16_t address, int32_t data);

	private:
		// escape, checksum and write..
		void ecWrite (char *msg);
		// read and test checksum, with expected length
		void ecRead (char *msg, int len);

		void writeMsg (char op, int16_t address);
		void writeMsg (char op, int16_t address, char *data, int len);

		void readStatus ();

		rts2core::ValueInteger *dPos;
		rts2core::ValueInteger *aPos;
		rts2core::ValueInteger *posErr;
		rts2core::ValueInteger *maxPosErr;

		rts2core::ValueDouble *dSpeed;
		rts2core::ValueInteger *dSpeedInt;
		rts2core::ValueDouble *aSpeed;
		rts2core::ValueInteger *aSpeedInt;
		rts2core::ValueDouble *maxSpeed;

		rts2core::ValueDouble *accel;
		rts2core::ValueDouble *decel;
		rts2core::ValueDouble *emerDecel;

		rts2core::ValueFloat *dCur;
		rts2core::ValueFloat *aCur;

		rts2core::ValueFloat *positionKp;
		rts2core::ValueFloat *speedKp;
		rts2core::ValueFloat *speedKi;
		rts2core::ValueFloat *speedImax;
		rts2core::ValueFloat *qCurrentKp;
		rts2core::ValueFloat *qCurrentKi;
		rts2core::ValueFloat *dCurrentKp;
		rts2core::ValueFloat *dCurrentKi;

		rts2core::ValueInteger *tgaMode;
		rts2core::ValueInteger *appStatus;
		rts2core::ValueInteger *faults;
		rts2core::ValueInteger *masterCmd;

		rts2core::ValueInteger *firmware;

		bool stopped;
		int32_t stoppedPosition;
};

}
