/* 
 * Driver for SSP5 Optec photometer, connected over serial port.
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


#include "phot.h"
#include "../utils/rts2connserial.h"

#include <time.h>

namespace rts2phot
{

/**
 * Driver for Optec SSP5 photometer, connected over serial port.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SSP5:public Rts2DevPhot
{
	private:
		const char *photFile;
		Rts2ConnSerial *photConn;

	protected:
		virtual int processOption (int _opt);

		virtual int init ();

		virtual int startIntegrate ();

	public:
		SSP5 (int argc, char **argv);

		virtual int scriptEnds ();

		virtual long getCount ();

		virtual int homeFilter ();
		virtual int startFilterMove (int new_filter);
		virtual long isFilterMoving ();
};

}

using namespace rts2phot;


int
SSP5::processOption (int _opt)
{
	switch (_opt)
	{
		case 'f':
			photFile = optarg;
			break;
		default:
			return Rts2DevPhot::processOption (_opt);
	}
	return 0;
}

int
SSP5::init ()
{
	char rbuf[10];
	int ret;
	ret = Rts2DevPhot::init ();
	if (ret)
		return ret;

	photConn = new Rts2ConnSerial (photFile, this, BS19200, C8, NONE, 40);
	photConn->setDebug (true);
	ret = photConn->init ();
	if (ret)
		return ret;
	
	if (photConn->writeRead ("SSMODE", 6, rbuf, 10, '\r') < 0)
		return -1;
	if (rbuf[0] != '!')
	  	return -1;
	return 0;
}

SSP5::SSP5 (int argc, char **argv):Rts2DevPhot (argc, argv)
{
	photType = "SSP5";
	photFile = "/dev/ttyS0";

	filter->addSelVal ("Dark");
	filter->addSelVal ("U");
	filter->addSelVal ("u");
	filter->addSelVal ("v");
	filter->addSelVal ("b");
	filter->addSelVal ("y");

	addOption ('f', NULL, 1, "serial port (default to /dev/ttyS0");
}


int
SSP5::scriptEnds ()
{
	startFilterMove (0);
	return Rts2DevPhot::scriptEnds ();
}


long
SSP5::getCount ()
{
	sendCount (10, req_time, false);
	return (long (req_time * USEC_SEC));
}


int
SSP5::homeFilter ()
{
	return photConn->writePort ("SHOMEx", 6);
}


int
SSP5::startIntegrate ()
{
	return 0;
}


int
SSP5::startFilterMove (int new_filter)
{
	char buf[9];
	strcpy (buf, "SFILTx\n\r");
	if (new_filter > 5 || new_filter < 0)
	{
		new_filter = 0;
		filter->setValueInteger (0);
	}
	buf[5] = new_filter + '1';
	if (photConn->writeRead (buf, 8, buf, 9, '\r') != 1)
		return -1;
	if (buf[0] != '!')
		return -1;
	return Rts2DevPhot::startFilterMove (new_filter);
}


long
SSP5::isFilterMoving ()
{
	return 0;
}


int
main (int argc, char **argv)
{
	SSP5 device = SSP5 (argc, argv);
	return device.run ();
}
