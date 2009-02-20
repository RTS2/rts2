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

		rts2core::ConnModbus *zelioConn;

	protected:
		virtual int processOption (int in_opt);

	public:
		Zelio (int argc, char **argv);
		virtual ~Zelio (void);
		virtual int init ();

		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();
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
main (int argc, char **argv)
{
	Zelio device = Zelio (argc, argv);
	return device.run ();
}
