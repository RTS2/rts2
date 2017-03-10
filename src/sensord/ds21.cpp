/* 
 * Driver for Ealing DS-21 motor controller.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "sensord.h"
#include "connection/serial.h"

namespace rts2sensord
{

class DS21;

/**
 * Holds values and perform operations on them for one axis.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DS21Axis
{
	public:
		/**
		 * @param in_anum Number of the axis being constructed.
		 */
		DS21Axis (DS21 *in_master, char in_anum);
		/**
		 * @return -3 when value was not found in this axis.
		 */
		int setValue (rts2core::Value *old_value, rts2core::Value *new_value);
		int info ();

		/**
		 * Check if axis is still moving.
		 *
		 * @return true if axis is still moving.
		 */
		bool isMoving ();

		void updateStatus ();

	private:
		DS21 *master;
		char anum;

		rts2core::ValueBool *enabled;
		rts2core::ValueLong *position;
		rts2core::ValueLong *poserr;
		rts2core::ValueInteger *velocity;
		rts2core::ValueInteger *acceleration;
		rts2core::ValueInteger *status;

		rts2core::ValueBool *limitSwitch;

		rts2core::ValueString *commandSet;
};

/**
 *
 */

class DS21: public Sensor
{
	public:
		DS21 (int argc, char **argv);
		virtual ~DS21 (void);

		template < typename T > void createAxisValue( T * &val, char anum, const char *in_val_name, const char *in_desc, bool writeToFits, int flags = 0)
		{
			char *n = new char[strlen (in_val_name) + 3];
			n[0] = anum + '0';
			n[1] = '.';
			strcpy (n+2, in_val_name);
			createValue (val, n, in_desc, writeToFits, flags);
			delete []n;
		}

		virtual int info ();

		/**
		 * Handles camera commands.
		 */
		virtual int commandAuthorized (rts2core::Rts2Connection * conn);

		int writePort (char anum, const char *msg);
		int readPort (char *buf, int blen);

		int writeReadPort (char anum, const char *msg, char *buf, int blen);

		int writeValue (char anum, const char cmd[3], rts2core::Value *val);
		int readValue (char anum, const char *cmd, rts2core::Value *val);
		/**
		 * Reads binary value.
		 */
		int readValueBin (char anum, const char *cmd, rts2core::Value *val);

		/**
		 * Called when axis start moving.
		 */
		void startMove ()
		{
			blockExposure ();
			setTimeout (1);
		}

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int idle ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		const char *dev;
		rts2core::ConnSerial *ds21;

		std::list <DS21Axis> axes;

		int home ();

		friend class DS21Axis;
};

};

using namespace rts2sensord;

DS21Axis::DS21Axis (DS21 *in_master, char in_anum)
{
	master = in_master;
	anum = in_anum;

	master->createAxisValue (enabled, anum, "enabled", "if motor is enabled", false, RTS2_VALUE_WRITABLE);
	master->createAxisValue (position, anum, "POSITION", "motor position", true, RTS2_VALUE_WRITABLE);
	master->createAxisValue (poserr, anum, "POS_ERROR", "motor position error", true);
	master->createAxisValue (velocity, anum, "velocity", "programmed velocity", false, RTS2_VALUE_WRITABLE);
	master->createAxisValue (acceleration, anum, "acceleration", "programmed acceleration", false, RTS2_VALUE_WRITABLE);
	master->createAxisValue (status, anum, "status", "drives status", false, RTS2_DT_HEX);
	master->createAxisValue (limitSwitch, anum, "limitSwitch", "true if limit switchs are active", false, RTS2_VALUE_WRITABLE);

	master->createAxisValue (commandSet, anum, "commands", "commands for this motor", false);

	master->writePort (anum, "RE");

	master->readValue (anum, "HE", commandSet);

	if (master->readValueBin (anum, "TS", status) == 0)
	{
		enabled->setValueBool (status->getValueInteger () & 0x0001);
	}
}

int DS21Axis::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == enabled)
	{
		if (master->writePort (anum, ((rts2core::ValueBool *) new_value)->getValueBool () ? "MN" : "MF"))
			return -2;
		updateStatus ();
		return 0;
	}
	if (old_value == position)
	{
		if (master->writeValue (anum, "MA", new_value))
			return -2;
		master->startMove ();
		return 0;
	}
	if (old_value == velocity)
	{
		return (master->writeValue (anum, "SV", new_value)) ? -2 : 0;
	}
	if (old_value == acceleration)
	{
		return (master->writeValue (anum, "SA", new_value)) ? -2 : 0;
	}
	if (old_value == limitSwitch)
	{
		return (master->writePort (anum, ((rts2core::ValueBool *) new_value)->getValueBool () ? "LN" : "LF")) ? -2 : 0;
	}
	return -3;
}

int DS21Axis::info ()
{
	master->readValue (anum, "TP", position);
	master->readValue (anum, "TE", poserr);
	master->readValue (anum, "GV", velocity);
	master->readValue (anum, "GA", acceleration);
	master->readValueBin (anum, "TS", status);
	return 0;
}

bool DS21Axis::isMoving ()
{
	if (master->readValueBin (anum, "TS", status))
		return false;
	if (status->getValueInteger () & 0x0100)
	{
		master->sendValueAll (status);
		master->readValue (anum, "TP", position);
		master->sendValueAll (position);
		return true;
	}
	return false;
}

void DS21Axis::updateStatus ()
{
	master->readValueBin (anum, "TS", status);
	master->sendValueAll (status);
}

int DS21::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int DS21::init ()
{
	int ret;

	ret = Sensor::init ();
	if (ret)
		return ret;

	ds21 = new rts2core::ConnSerial (dev, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 20);
	ret = ds21->init ();
	if (ret)
		return ret;

	ds21->flushPortIO ();

	writePort (0, "RE");

	axes.push_back (DS21Axis (this, 5));
	axes.push_back (DS21Axis (this, 3));

	return 0;
}

int DS21::idle ()
{
	if (blockingExposure ())
	{
		for (std::list <DS21Axis>::iterator iter = axes.begin (); iter != axes.end (); iter++)
		{
			if ((*iter).isMoving ())
				return Sensor::idle ();
		}
		clearExposure ();
		for (std::list <DS21Axis>::iterator iter = axes.begin (); iter != axes.end (); iter++)
		{
			(*iter).updateStatus ();
		}
		setTimeout (10 * USEC_SEC);
	}
	return Sensor::idle ();
}

int DS21::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	for (std::list <DS21Axis>::iterator iter = axes.begin (); iter != axes.end (); iter++)
	{
		int ret = (*iter).setValue (old_value, new_value);
		if (ret != -3)
			return ret;
	}
	return Sensor::setValue (old_value, new_value);
}

DS21::DS21 (int argc, char **argv):Sensor (argc, argv)
{
	dev = "/dev/ttyS0";
	ds21 = NULL;

	addOption ('f', NULL, 1, "device serial port, default to /dev/ttyS0");
}

DS21::~DS21 (void)
{
	delete ds21;
}

int DS21::info ()
{
	int ret;
	for (std::list <DS21Axis>::iterator iter = axes.begin (); iter != axes.end (); iter++)
	{
		ret = (*iter).info ();
		if (ret)
			return ret;
	}
	return Sensor::info ();
}

int DS21::home ()
{
	return writePort (0, "GH");
}

int DS21::commandAuthorized (rts2core::Rts2Connection *conn)
{
	if (conn->isCommand ("home"))
	{
		if (!conn->paramEnd ())
			return -2;
		return home ();
	}
	return Sensor::commandAuthorized (conn);
}

int DS21::writePort (char anum, const char *msg)
{
	int blen = strlen (msg);
	char *buf = new char[blen + 4];
	buf[0] = anum + '0';
	strcpy (buf + 1, msg);
	#ifdef DEBUG_ALL
	logStream (MESSAGE_DEBUG) << "write on port " << buf << sendLog;
	#endif
	buf[blen + 1] = '\r';
	buf[blen + 2] = '\n';
	buf[blen + 3] = '\0';
	return ds21->writePort (buf, blen + 3);
}

int DS21::readPort (char *buf, int blen)
{
	int ret = ds21->readPort (buf, blen, '\n');
	if (ret == -1)
		return ret;
	// delete end '\r'
	if (buf[ret - 2] == '\r')
		buf[ret - 2] = '\0';
	return 0;
}

int DS21::writeReadPort (char anum, const char *msg, char *buf, int blen)
{
	int ret;
	ret = writePort (anum, msg);
	if (ret)
		return ret;
	return readPort (buf, blen);

}

int DS21::writeValue (char anum, const char cmd[3], rts2core::Value *val)
{
	int ret;
	const char *sval = val->getValue ();
	char *buf = new char[strlen (sval) + 3];

	memcpy (buf, cmd, 2);
	strcpy (buf + 2, sval);

	ret = writePort (anum, buf);
	delete[] buf;
	return ret;
}

int DS21::readValue (char anum, const char *cmd, rts2core::Value *val)
{
	int ret;
	char buf[500];

	ds21->flushPortIO ();

	ret = writeReadPort (anum, cmd, buf, 500);
	if (ret)
		return -1;
	return val->setValueCharArr (buf);
}

int DS21::readValueBin (char anum, const char *cmd, rts2core::Value *val)
{
	int ret;
	char buf[500];

	ds21->flushPortIO ();

	ret = writeReadPort (anum, cmd, buf, 500);
	if (ret)
		return -1;
	// convert bin to number
	for (char *top = buf; *top; top++)
	{
		switch (*top)
		{
			case '0':
				ret = ret << 1;
				break;
			case '1':
				ret = ret << 1;
				ret |= 0x01;
				break;
		}
	}
	return val->setValueInteger (ret);
}

int main (int argc, char **argv)
{
	DS21 device = DS21 (argc, argv);
	return device.run ();
}
