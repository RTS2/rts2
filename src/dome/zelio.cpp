/* 
 * Driver for IR (OpenTPL) dome.
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

#include "dome.h"
#include "../utils/connmodbus.h"

namespace rts2dome
{

/**
 * Driver for Bootes IR dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Zelio:public Dome
{
	private:
		HostString *host;

		Rts2ValueInteger *J1XT1;
		Rts2ValueInteger *J2XT1;
		Rts2ValueInteger *J3XT1;
		Rts2ValueInteger *J4XT1;

		Rts2ValueInteger *O1XT1;
		Rts2ValueInteger *O2XT1;
		Rts2ValueInteger *O3XT1;
		Rts2ValueInteger *O4XT1;

		rts2core::ConnModbus *zelioConn;

	protected:
		virtual int processOption (int in_opt);

		virtual int init ();

		virtual int setValue (Rts2Value *oldValue, Rts2Value *newValue);

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();
	public:
		Zelio (int argc, char **argv);
		virtual ~Zelio (void);

		virtual int info ();
};

}

using namespace rts2dome;

int
Zelio::startOpen ()
{
	return 0;
}


long
Zelio::isOpened ()
{
	return -2;
}


int
Zelio::endOpen ()
{
	return 0;
}


int
Zelio::startClose ()
{
	return 0;
}


long
Zelio::isClosed ()
{
	return -2;
}


int
Zelio::endClose ()
{
	return 0;
}


int
Zelio::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'z':
			host = new HostString (optarg, "502");
			break;
		default:
			return Dome::processOption (in_opt);
	}
	return 0;
}


Zelio::Zelio (int argc, char **argv)
:Dome (argc, argv)
{
	createValue (J1XT1, "J1XT1", "first input", false, RTS2_DT_HEX);
	createValue (J2XT1, "J2XT1", "second input", false, RTS2_DT_HEX);
	createValue (J3XT1, "J3XT1", "third input", false, RTS2_DT_HEX);
	createValue (J4XT1, "J4XT1", "fourth input", false, RTS2_DT_HEX);

	createValue (O1XT1, "O1XT1", "first output", false, RTS2_DT_HEX);
	createValue (O2XT1, "O2XT1", "second output", false, RTS2_DT_HEX);
	createValue (O3XT1, "O3XT1", "third output", false, RTS2_DT_HEX);
	createValue (O4XT1, "O4XT1", "fourth output", false, RTS2_DT_HEX);

	host = NULL;

	addOption ('z', NULL, 1, "Zelio TCP/IP address and port (separated by :)");
}


Zelio::~Zelio (void)
{
	delete zelioConn;
	delete host;
}


int
Zelio::info ()
{
	uint16_t ret[8];
	zelioConn->readHoldingRegisters (16, 8, ret);

	J1XT1->setValueInteger (ret[0]);
	J2XT1->setValueInteger (ret[1]);
	J3XT1->setValueInteger (ret[2]);
	J4XT1->setValueInteger (ret[3]);

	O1XT1->setValueInteger (ret[4]);
	O2XT1->setValueInteger (ret[5]);
	O3XT1->setValueInteger (ret[6]);
	O4XT1->setValueInteger (ret[7]);

	zelioConn->readHoldingRegisters (32, 4, ret);
	return Dome::info ();
}


int
Zelio::init ()
{
	int ret = Dome::init ();
	if (ret)
		return ret;

	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify zeliho hostname (with -z option)." << sendLog;
		return -1;
	}
	zelioConn = new rts2core::ConnModbus (this, host->getHostname (), host->getPort ());
	zelioConn->setDebug (true);
	return zelioConn->init ();
}


int
Zelio::setValue (Rts2Value *oldValue, Rts2Value *newValue)
{
	if (oldValue == J1XT1)
		return zelioConn->writeHoldingRegister (16, newValue->getValueInteger ()) == 0 ? 0 : -2;
	if (oldValue == J2XT1)
		return zelioConn->writeHoldingRegister (17, newValue->getValueInteger ()) == 0 ? 0 : -2;
	if (oldValue == J3XT1)
		return zelioConn->writeHoldingRegister (18, newValue->getValueInteger ()) == 0 ? 0 : -2;
	if (oldValue == J4XT1)
		return zelioConn->writeHoldingRegister (19, newValue->getValueInteger ()) == 0 ? 0 : -2;
	return Dome::setValue (oldValue, newValue);
}


int
main (int argc, char **argv)
{
	Zelio device = Zelio (argc, argv);
	return device.run ();
}
