/* 
 * Driver for TBT Zelio
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

#include "sensord.h"
#include "connection/modbus.h"

// Zelio registers

#define ZREG_J1XT1       16
#define ZREG_J2XT1       17
#define ZREG_J3XT1       18
#define ZREG_J4XT1       19

#define ZREG_O1XT1       20
#define ZREG_O2XT1       21
#define ZREG_O3XT1       22
#define ZREG_O4XT1       23

namespace rts2sensord
{

/**
 * TBT Zelio control.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TBT:public Sensor
{
	public:
		TBT (int argc, char **argv);
		virtual ~TBT (void);

		virtual int info ();

	protected:
		virtual int processOption (int in_opt);

		virtual int initHardware ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

	private:
		HostString *host;

		rts2core::ValueInteger *J1XT1;
		rts2core::ValueInteger *J2XT1;
		rts2core::ValueInteger *J3XT1;
		rts2core::ValueInteger *J4XT1;

		rts2core::ValueInteger *O1XT1;
		rts2core::ValueInteger *O2XT1;
		rts2core::ValueInteger *O3XT1;
		rts2core::ValueInteger *O4XT1;

		rts2core::ConnModbus *zelioConn;
};

}

using namespace rts2sensord;

TBT::TBT (int argc, char **argv):Sensor (argc, argv)
{
	zelioConn = NULL;
	host = NULL;

	createValue (J1XT1, "J1XT1", "first input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);
	createValue (J2XT1, "J2XT1", "second input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);
	createValue (J3XT1, "J3XT1", "third input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);
	createValue (J4XT1, "J4XT1", "fourth input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);

	createValue (O1XT1, "O1XT1", "first output", false, RTS2_DT_HEX);
	createValue (O2XT1, "O2XT1", "second output", false, RTS2_DT_HEX);
	createValue (O3XT1, "O3XT1", "third output", false, RTS2_DT_HEX);
	createValue (O4XT1, "O4XT1", "fourth output", false, RTS2_DT_HEX);

	addOption ('z', NULL, 1, "Zelio TCP/IP address and port (separated by :)");
}

TBT::~TBT (void)
{
	delete zelioConn;
	delete host;
}

int TBT::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'z':
			host = new HostString (optarg, "502");
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int TBT::info ()
{
	uint16_t regs[8];
	try
	{
		zelioConn->readHoldingRegisters (16, 8, regs);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << "info " << err << sendLog;
		return -1;
	}

	J1XT1->setValueInteger (regs[0]);
	J2XT1->setValueInteger (regs[1]);
	J3XT1->setValueInteger (regs[2]);
	J4XT1->setValueInteger (regs[3]);

	sendValueAll (J1XT1);
	sendValueAll (J2XT1);
	sendValueAll (J3XT1);
	sendValueAll (J4XT1);

	O1XT1->setValueInteger (regs[4]);
	O2XT1->setValueInteger (regs[5]);
	O3XT1->setValueInteger (regs[6]);
	O4XT1->setValueInteger (regs[7]);

	sendValueAll (O1XT1);
	sendValueAll (O2XT1);
	sendValueAll (O3XT1);
	sendValueAll (O4XT1);

	return Sensor::info ();
}

int TBT::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify zelio hostname (with -z option)." << sendLog;
		return -1;
	}
	zelioConn = new rts2core::ConnModbus (this, host->getHostname (), host->getPort ());
	
	uint16_t regs[8];

	try
	{
		zelioConn->init ();
		zelioConn->readHoldingRegisters (16, 8, regs);
	}
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << "initHardware " << er << sendLog;
		return -1;
	}

	int ret = info ();
	if (ret)
		return ret;

	return 0;
}

int TBT::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	try
	{
		if (oldValue == J1XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J1XT1, newValue->getValueInteger ());
			return 0;
		}
		else if (oldValue == J2XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J2XT1, newValue->getValueInteger ());
			return 0;
		}
		else if (oldValue == J3XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J3XT1, newValue->getValueInteger ());
			return 0;
		}
		else if (oldValue == J4XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J4XT1, newValue->getValueInteger ());
			return 0;
		}
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << "setValue " << oldValue->getName () << " " << err << sendLog;
		return -2;
	}
	return Sensor::setValue (oldValue, newValue);
}

int main (int argc, char **argv)
{
	TBT device (argc, argv);
	return device.run ();
}
