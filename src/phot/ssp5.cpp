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
#include "connection/serial.h"

#include <time.h>

using namespace rts2phot;

/**
 * Driver for Optec SSP5 photometer, connected over serial port.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SSP5:public Photometer
{
	public:
		SSP5 (int argc, char **argv);

	protected:
		virtual int processOption (int _opt);
		virtual int init ();
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int setExposure (float _exp);

		virtual int startIntegrate ();

		virtual int scriptEnds ();

		virtual long getCount ();

		virtual int homeFilter ();
		virtual int startFilterMove (int new_filter);
		virtual long isFilterMoving ();

	private:
		const char *photFile;
		rts2core::ConnSerial *photConn;

		rts2core::ValueSelection *gain;
};

int SSP5::processOption (int _opt)
{
	switch (_opt)
	{
		case 'f':
			photFile = optarg;
			break;
		default:
			return Photometer::processOption (_opt);
	}
	return 0;
}

int SSP5::init ()
{
	char rbuf[10];
	int ret;
	ret = Photometer::init ();
	if (ret)
		return ret;

	photConn = new rts2core::ConnSerial (photFile, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 40);
	photConn->setDebug (true);
	ret = photConn->init ();
	if (ret)
		return ret;
	if (photConn->writePort ("SS", 2) < 0)
		return -1;
	if (photConn->writeRead ("SSMODE", 8, rbuf, 10, '\r') < 0)
		return -1;
	if (rbuf[0] != '!')
	  	return -1;
	if (photConn->writePort ("SS", 2) < 0)
		return -1;
	return 0;
}

int SSP5::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == gain)
	{
		char buf[6];
		strcpy (buf, "SGAIN");
		buf[5] = newValue->getValueInteger () + '1';
		if (photConn->writeRead (buf, 6, buf, 5, '\r') < 0)
			return -2;
		if (buf[0] != '!')
			return -2;
		return 0;
	}
	if (oldValue == filter)
	{
		return startFilterMove (newValue->getValueInteger ()) == 0 ? 0 : -2;
	}
	return Photometer::setValue (oldValue, newValue);
}

int SSP5::setExposure (float _exp)
{
	char buf[50];
	snprintf (buf, 7, "SI%04i", int (getExposure () / 0.01));
	if (photConn->writeRead (buf, 6, buf, 10, '\r') < 0)
		return -1;
	if (buf[0] != '!')
		return -1;
	return Photometer::setExposure (_exp);
}

SSP5::SSP5 (int argc, char **argv):Photometer (argc, argv)
{
	photType = "SSP5";
	photFile = "/dev/ttyS0";

	filter->addSelVal ("y");
	filter->addSelVal ("b");
	filter->addSelVal ("v");
	filter->addSelVal ("u");
	filter->addSelVal ("U");
	filter->addSelVal ("Dark");

	createValue (gain, "gain", "photometer gain", true, RTS2_VALUE_WRITABLE);
	gain->addSelVal ("100");
	gain->addSelVal ("10");
	gain->addSelVal ("1");

	addOption ('f', NULL, 1, "serial port (default to /dev/ttyS0");
}

int SSP5::scriptEnds ()
{
	startFilterMove (0);
	return Photometer::scriptEnds ();
}

long SSP5::getCount ()
{
	int ret;
	char buf[10];
	int oldVtime = photConn->getVTime ();
	ret = photConn->setVTime (req_time * 10 + 2);
	if (ret)
		return -1;
	ret = photConn->writeRead ("SCOUNT", 6, buf, 10, '\r');
	photConn->setVTime (oldVtime);
	if (ret < 0)
	{
		photConn->flushPortIO ();
		return -1;
	}
	if (!(buf[0] == 'C' && buf[1] == '=' && buf[7] == '\n' && buf[8] == '\r'))
	{
		if (strncmp (buf, "ER=", 3) == 0 && buf[3] == '2')
		{
			// overflow
			sendCount (0, req_time, true);
			photConn->flushPortIO ();
			return 0;
		}
		photConn->flushPortIO ();
		return -1;
	}
	if (buf[0] == '!')
		return -1;
	buf[7] = '\0';
	sendCount (atoi (buf + 2), req_time, false);
	return 0;
}

int SSP5::homeFilter ()
{
	return photConn->writePort ("SHOMEx", 6);
}

int SSP5::startIntegrate ()
{
	// set integration time..
	char buf[50];
	if (req_count->getValueInteger () <= 0)
		return -1;
	snprintf (buf, 7, "SI%04i", int (getExposure () / 0.01));
	if (photConn->writeRead (buf, 6, buf, 10, '\r') < 0)
		return -1;
	if (buf[0] != '!')
		return -1;
	return 0;
}

int SSP5::startFilterMove (int new_filter)
{
	char buf[6];
	strcpy (buf, "SFILT");
	if (new_filter > 5 || new_filter < 0)
	{
		new_filter = 0;
		filter->setValueInteger (0);
	}
	buf[5] = new_filter + '1';
	if (photConn->writeRead (buf, 6, buf, 6, '\r') < 0)
		return -1;
	if (buf[0] != '!')
		return -1;
	return Photometer::startFilterMove (new_filter);
}

long SSP5::isFilterMoving ()
{
	return 0;
}

int main (int argc, char **argv)
{
	SSP5 device (argc, argv);
	return device.run ();
}
