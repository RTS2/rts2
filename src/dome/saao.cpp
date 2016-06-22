/* 
 * Driver for SAAO Delta-DVP Series PLC copula
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

#include "cupola.h"
#include "connection/serial.h"

namespace rts2dome
{

/**
 * Driver for DELTA DVP Series PLC controlling 1m SAAO dome.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class SAAO:public Cupola
{
	public:
		SAAO (int argc, char **argv);
		virtual ~SAAO ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

	private:
		const char *device_file;

		rts2core::ConnSerial *domeConn;
		
};

}

using namespace rts2dome;

SAAO::SAAO (int argc, char **argv):Cupola (argc, argv)
{
	device_file = "/dev/ttyS0";
	domeConn = NULL;

	addOption ('f', NULL, 1, "path to device file, default is /dev/ttyS0");
}

SAAO::~SAAO ()
{
	delete domeConn;
}

int SAAO::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Cupola::processOption (opt);
	}
	return 0;
}

int SAAO::initHardware ()
{
	domeConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C7, rts2core::EVEN, 50);
	domeConn->setDebug (getDebug ());
	int ret = domeConn->init ();
	if (ret)
		return ret;
	return 0;
}

int main (int argc, char **argv)
{
}
