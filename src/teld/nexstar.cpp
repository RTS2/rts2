/* 
 * NexStar telescope driver.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#include "teld.h"
#include "../../lib/rts2/connserial.h"
#include "configuration.h"

/*!
 * NexStar teld for testing purposes.
 */

namespace rts2teld
{

/**
 * NexStar telescope driver. The protocol is documented on:
 * http://www.celestron.com/c3/images/files/downloads/1154108406_nexstarcommprot.pdf
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class NexStar:public Telescope
{
	public:
	        NexStar (int argc, char **argv);
		~NexStar ();

        	virtual int processOption (int in_opt);
		virtual int startResync ()
		{
			return 0;
		}

		virtual int startMoveFixed (double tar_az, double tar_alt)
		{
			return 0;
		}

		virtual int stopMove ()
		{
			return 0;
		}

		virtual int startPark ()
		{
			return 0;
		}

		virtual int endPark ()
		{
			return 0;
		}

	protected:
		virtual int init ();
		virtual int initValues ();

		virtual int info ();

		virtual int isMoving ()
		{
			return -2;
		}

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
		const char *serialPort;
		rts2core::ConnSerial *serial;

		// retrieve degrees
		void getDeg (char command, double &d1, double &d2);

		// retrieve precise degrees
		void getPreciseDeg (char command, double &d1, double &d2);

		char vmajor, vminor;

		rts2core::ValueString *version;
};

}

using namespace rts2teld;

NexStar::NexStar (int argc, char **argv):Telescope (argc,argv)
{
	serialPort = "/dev/ttyS0";
	serial = NULL;

	createValue (version, "version", "NexStar version", false);

	addOption ('f', NULL, 1, "path to NexStar serial port (defaults to /dev/ttyS0)");
}

NexStar::~NexStar ()
{
	delete serial;
}

int NexStar::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			serialPort = optarg;
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int NexStar::init ()
{
	int ret = Telescope::init ();
	if (ret)
		return ret;

	serial = new rts2core::ConnSerial (serialPort, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 90);
	serial->setDebug ();

	ret = serial->init ();
	if (ret)
		return ret;

	char rbuf[4];

	ret = serial->writeRead ("V", 1, rbuf, 3, '#');
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot read version" << sendLog;
		return -1;
	}

	vmajor = rbuf[0] - '0';
	vminor = rbuf[1] - '0';

	rbuf[2] = rbuf[1];
	rbuf[1] = '.';

	version->setValueCharArr (rbuf);

	ret = info ();
	if (ret)
		return ret;
	return 0;
}

int NexStar::initValues ()
{
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
	strcpy (telType, "NexStar");
	return Telescope::initValues ();
}

int NexStar::info ()
{
	double ra, dec;
	getPreciseDeg ('e', ra, dec);
	setTelRa (ra);
	setTelDec (dec);
	return Telescope::info ();
}

void NexStar::getDeg (char command, double &d1, double &d2)
{
	char wbuf[50];
	int ret = serial->writeRead (&command, 1, wbuf, 50, '#');
	if (ret < 0)
		throw rts2core::Error ("cannot get precise degrees information");
	int32_t i1, i2;
	ret = sscanf (wbuf, "%x,%x#", &i1, &i2);
	if (ret != 2)
	{
		throw rts2core::Error (std::string ("invalid return ") + wbuf); 
	}
	d1 = 360.0 * ((double) i1) / 0xffff;
	d2 = 360.0 * ((double) i2) / 0xffff;
}

void NexStar::getPreciseDeg (char command, double &d1, double &d2)
{
	char wbuf[50];
	int ret = serial->writeRead (&command, 1, wbuf, 50, '#');
	if (ret < 0)
		throw rts2core::Error ("cannot get precise degrees information");
	int32_t i1, i2;
	ret = sscanf (wbuf, "%x,%x#", &i1, &i2);
	if (ret != 2)
	{
		throw rts2core::Error (std::string ("invalid return ") + wbuf); 
	}
	d1 = 360.0 * ((double) i1) / 0xffffffff;
	d2 = 360.0 * ((double) i2) / 0xffffffff;
}

int main (int argc, char **argv)
{
	NexStar device (argc, argv);
	return device.run ();
}
