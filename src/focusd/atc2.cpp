/*
 * Copyright (C) 2010 Francisco Foster Buron
 * Copyright (C) 2010 Petr Kubanek, Instritute of Physics CR <kubanek@fzu.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define FOCUSER_PORT "/dev/ttyS1"
#define OPT_HOME     OPT_LOCAL + 236

#include "focusd.h"
#include "connection/serial.h"

// AVAILABLE COMMANDS IN ATC-02:
//     OPENREM   +(CR/LF)   open communication between C and ATC-02
//                             OPENREM   +(CR/LF)
//     CLOSEREM  +(CR/LF)   close communcation between C and ATC-02
//                             CLOSEREM  +(CR/LF)
//     UPDATEPC  +(CR/LF)   request for status of system parameters
//                             STABT xx.0+(CR/LF)   mirror temperature target
//                             SETFAN xxx+(CR/LF)   fan speed
//                             PRITExxxxx+(CR/LF)   primary mirror temperature
//                             SECTExxxxx+(CR/LF)   secondary mirror temperature
//                             BFL xxx.xx+(CR/LF)   back focus length in mm
//                             SHUTTER  x+(CR/LF)   shutter status (0 or 1)
//                             AMBTE-xx.x+(CR/LF)   ambient temperature
//                             HUMID xx.x+(CR/LF)   ambient humidity
//                             PRESxxxx.x+(CR/LF)   ambient pressure
//                             DEWPOxxx.x+(CR/LF)   dew point
//     READSETT  +(CR/LF)   request for status of ATC-02 firmware parameters
//                             VERSIONx.x+(CR/LF)   firmware version
//                             WARMFLAG x+(CR/LF)   status of temperature stabilization (0 heating is off, 1 is regulating)
//                             UNIT     x+(CR/LF)   units for temperature (C or F)
//                             DELTAMAXxx+(CR/LF)   maximum shift allowed from optimal position
//                             OPBFxxx.xx+(CR/LF)   optimal position for ideal mirrors distance
//                             40 x's+(CR/LF)       user/factory info
//                             40 x's+(CR/LF)       user/factory info
//     WARMTOGGLE+(CR/LF)   toggle (invert) the status of heating. After PC connection status is OFF
//                             WARMON    +(CR/LF)   heating is active
//                             WAMROFF   +(CR/LF)   heating is inactive
//     SETFAN xxx+(CR/LF)
//                             SETFAN xxx+(CR/LF)
//     SETSTABxxx+(CR/LF)
//                             SETSTABxxx+(CR/LF)
//     BFL xxx.xx+(CR/LF)
//                             BFL xxx.xx+(CR/LF)
//     FINDOPTIMA+(CR/LF)
//                             FINDOPTIMA+(CR/LF)
//     OPENSHUTT +(CR/LF)
//                             OPENSHUTT +(CR/LF)
//     CLOSESHUTT+(CR/LF)
//                             CLOSESHUTT+(CR/LF)
//     TIMEhhmmss+(CR/LF)
//                             TIMEhhmmss+(CR/LF)
//     DATEddmmyy+(CR/LF)
//                             DATEddmmyy+(CR/LF)
namespace rts2focusd
{

class ATC2:public Focusd
{
	public:
		ATC2 (int argc, char **argv);
		~ATC2 (void);

		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};

		virtual int commandAuthorized (rts2core::Rts2Connection * conn);
	protected:
		virtual int processOption (int in_opt);
		virtual int isFocusing ();

		virtual bool isAtStartPosition ()
		{
			return false;
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);
	private:
		void getValue (const char *name, rts2core::Value *val);
		void openRem ();

		int findOptima ();
		
		char buf[15];
		const char *device_file;
		bool homeAtStart;
                rts2core::ConnSerial *ATC2Conn; // communication port with ATC2

		rts2core::ValueInteger *mirrorTarget;
		rts2core::ValueInteger *fanSpeed;
		rts2core::ValueFloat *primMirrorTemp;
		rts2core::ValueFloat *secMirrorTemp;
		rts2core::ValueBool *shutter;
		rts2core::ValueFloat *humidity;
		rts2core::ValueFloat *pressure;
		rts2core::ValueFloat *dewpoint;
};

}

using namespace rts2focusd;

ATC2::ATC2 (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;
	homeAtStart = false;
	ATC2Conn = NULL;

	createTemperature (); // ??

	createValue (mirrorTarget, "mirror_target", "[C] target mirror temperature", true, RTS2_VALUE_WRITABLE);
	createValue (fanSpeed, "fan_speed", "[rpm] fan speed", true, RTS2_VALUE_WRITABLE);
	createValue (primMirrorTemp, "temp_primary", "[C] primary mirror temperature", true);
	createValue (secMirrorTemp, "temp_secondary", "[C] secondary mirror temperature", true);
	createValue (shutter, "shutter", "[on/off] shutter position", true, RTS2_VALUE_WRITABLE);
	createValue (humidity, "humidity", "[%] relative humidity", true);
	createValue (pressure, "pressure", "[hP] atmospheric pressure", true);
	createValue (dewpoint, "dewpoint", "[C] dewpoint", true);

	addOption ('f', NULL, 1, "device file (ussualy /dev/ttySx");
	addOption (OPT_HOME, "home", 0, "home (send FINDOPTIMA) at startup");
}

ATC2::~ATC2 ()
{
	ATC2Conn->writeRead ("CLOSEREM  ", 10, buf, 14);
  	delete ATC2Conn;
}

int ATC2::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case OPT_HOME:
			homeAtStart = true;
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int ATC2::init ()
{
	int ret;

	ret = Focusd::init ();

	if (ret)
		return ret;

	ATC2Conn = new rts2core::ConnSerial (device_file, this, rts2core::BS2400, rts2core::C8, rts2core::NONE, 100);

	ret = ATC2Conn->init ();
	if (ret)
		return ret;

	ATC2Conn->flushPortIO ();

	openRem ();

	setIdleInfoInterval (40);

	if (homeAtStart)
	{
		ret = findOptima ();
		if (ret)
			return -1;
	}

	return 0;
}

int ATC2::initValues ()
{
	focType = std::string ("ATC2");
	return Focusd::initValues ();
}

void ATC2::getValue (const char *name, rts2core::Value *val)
{
	int ret = ATC2Conn->readPort (buf, 12, '\n');
	if (ret != 12)
		throw rts2core::Error ("cannot read reply from serial port");
	size_t l = strlen (name);
	if (strncmp (buf, name, l))
	  	throw rts2core::Error (std::string ("invalid reply ") + buf);
	if (val == NULL)
		return;
	if (val->getValueBaseType () == RTS2_VALUE_BOOL)
	{
		((rts2core::ValueBool *) val)->setValueBool (atoi (buf + l));
	}
	else
	{
		ret = val->setValueCharArr (buf + l);
		if (ret)
			throw rts2core::Error (std::string ("invalid value ") + buf);
	}
}

int ATC2::info ()
{
	int ret;
	ret = ATC2Conn->writeRead ("UPDATEPC  ", 10, buf, 2);
	if (ret != 2)
		return ret;
	try
	{
		getValue ("STABT", mirrorTarget);
		getValue ("SETFAN", fanSpeed);
		getValue ("PRITE", primMirrorTemp);
		getValue ("SECTE", secMirrorTemp);
		getValue ("BFL", position);
		getValue ("SHUTTER", shutter);
		getValue ("AMBTE", temperature);
		getValue ("HUMID", humidity);
		getValue ("PRES", pressure);
		getValue ("DEWPO", dewpoint);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		// assume that reply was CLOSEREM..
		openRem ();
		return -1;
	}

	return Focusd::info ();
}

int ATC2::commandAuthorized (rts2core::Rts2Connection * conn)
{
	if (conn->isCommand ("home"))
	{
		return findOptima () ? -2 : 0;
	}
	return Focusd::commandAuthorized (conn);
}

void ATC2::openRem ()
{
	int ret = ATC2Conn->writeRead ("OPENREM   ", 10, buf, 14);
	if (ret != 14)
		throw rts2core::Error ("OPENREM command returns invalid number of characters");
	if (strncmp (buf, "\r\nOPENREM   \r\n", 14))
		throw rts2core::Error (std::string ("invalid reply from OPENREM command :") + buf);
}

int ATC2::findOptima ()
{
	ATC2Conn->setVTime (400);
	int ret = ATC2Conn->writeRead ("FINDOPTIMA", 10, buf, 14);
	ATC2Conn->setVTime (100);
	if (ret != 14)
		return -1;
	if (strncmp (buf, "\r\nFINDOPTIMA\r\n", 14))
		return -1;
	return 0;
}

// send focus to given position
// Robofocus: FG000000 gets position, while FGXXXXXX sets position when XXXXXXis different from zero.
// ATC-02: BFL_xxx.xx+(CR/LF) set the new back focus position
int ATC2::setTo (double num)
{
	if (fabs (num - position->getValueDouble ()) < 0.015)
		return 0;
	char b1[10];
	char command[50];

	// compare as well strings we will send..
	snprintf (b1, 9, "%06.2f", num);
	snprintf (command, 9, "%06.2f", position->getValueDouble ());
	if (strcmp (b1, command) == 0)
		return 0;

	logStream (MESSAGE_DEBUG) << "changing position from " << position->getValueDouble () << " (" << command << ") to " << num << " (" << b1 << ")" << sendLog;

	size_t l = snprintf (command, 50, "BFL %06.2f", num);
	if (ATC2Conn->writePort (command, l))
		return -1;
	ATC2Conn->setVTime (1000);
	if (ATC2Conn->readPort (command, 14) != 14)
		return -1;
	ATC2Conn->setVTime (100);
	return 0;
}

int ATC2::isFocusing ()
{
	return -2;
}

int ATC2::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == fanSpeed)
	{
		if (newValue->getValueInteger () > 100 || newValue->getValueInteger () < 0)
			return -2;
		int l = snprintf (buf, 11, "SETFAN %03i", newValue->getValueInteger ());
		l = ATC2Conn->writeRead (buf, 10, buf, 14);
		if (l != 14)
			return -2;
		return 0;
	}
	if (oldValue == mirrorTarget)
	{
		if (newValue->getValueInteger () > temperature->getValueDouble () + 10)
		{
			logStream (MESSAGE_ERROR) << "too high target temperature - " << newValue->getValueInteger () << " " << temperature->getValueInteger () << sendLog;
			return -2;
		}
		int l = snprintf (buf, 11, "SETSTAB%+03i", newValue->getValueInteger ());
		l = ATC2Conn->writeRead (buf, 10, buf, 14);
		if (l != 14)
			return -2;
		return 0;
	}
	return Focusd::setValue (oldValue, newValue);
}

int main (int argc, char **argv)
{
	ATC2 device (argc, argv);
	return device.run ();
}
