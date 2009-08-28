/* 
 * Dome driver for Bootes 2 station.
 * Copyright (C) 2005-2008 Stanislav Vitek
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

#include <comedilib.h>
#include <ostream>

class ComediDOError
{
	private:
		const char *name;
	public:
		ComediDOError (const char *_name)
		{
			name = _name;
		}

		friend std::ostream & operator << (std::ostream & os, ComediDOError & err)
		{
			os << "Cannot read digital input " << err.name;
			return os;
		}
};


namespace rts2dome
{

/**
 * Class for control of the Bootes telescope roof. This class uses some PCI
 * board which is accesible from Comedi library.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @see htpp://www.comedi.org
 */
class IEEC: public Dome
{
	private:
		comedi_t *comediDevice;
		const char *comediFile;


		void comediReadDIO (int channel, Rts2ValueBool *val, const char *name);

	protected:
		virtual int processOption (int _opt);
		virtual int init ();
		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

	public:
		IEEC (int argc, char **argv);
		virtual ~IEEC ();
};

}

using namespace rts2dome;

void
IEEC::comediReadDIO (int channel, Rts2ValueBool *val, const char *name)
{
	int ret;
	unsigned int v;
	ret = comedi_dio_read (comediDevice, 3, channel, &v);
	if (ret != 1)
		throw ComediDOError (name);
	val->setValueBool (v != 0);
}


int
IEEC::processOption (int _opt)
{
	switch (_opt)
	{
		case 'c':
			comediFile = optarg;
			break;
		default:
			return Dome::processOption (_opt);
	}
	return 0;
}


int
IEEC::init ()
{
	int ret;
	ret = Dome::init ();
	if (ret)
		return ret;

	comediDevice = comedi_open (comediFile);
	if (comediDevice == NULL)
	{
		logStream (MESSAGE_ERROR) << "Cannot open comedi port " << comediFile << sendLog;
		return -1;
	}

	/* input port - sensors - subdevice 3 */
	int subdev = 3;
	for (int i = 0; i < 8; i++)
	{
		ret = comedi_dio_config (comediDevice, subdev, i, COMEDI_INPUT);
		if (ret != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot init comedi sensors - subdev " << subdev
				<< " channel " << i << ", error " << ret << sendLog;
			return -1;
		}
	}
	/* output port - roof - subdevice 2 */
	subdev = 2;
	for (int i = 0; i < 2; i++)
	{
		ret = comedi_dio_config (comediDevice, subdev, 0, COMEDI_OUTPUT);
		if (ret != 1)
		{
		  	logStream (MESSAGE_ERROR) << "Cannot init comedi roof - subdev " << subdev
				<< " channel 0, error " << ret << sendLog;
			return -1;
		}
	}

	return 0;
}


int
IEEC::info ()
{
	return Dome::info ();
}


int
IEEC::startOpen ()
{
	if (comedi_dio_write (comediDevice, 2, 0, 0) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot stop roof closing" << sendLog;
		return -1;
	}
	usleep (USEC_SEC / 2);
	if (comedi_dio_write (comediDevice, 2, 1, 1) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot start roof opening" << sendLog;
		return -1;
	}
	return 0;
}


long
IEEC::isOpened ()
{
}


int
IEEC::endOpen ()
{
	return 0;
}


int
IEEC::startClose ()
{
	if (comedi_dio_write (comediDevice, 2, 1, 0) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot stop roof opening" << sendLog;
	}
	usleep (USEC_SEC / 2);
	if (comedi_dio_write (comediDevice, 2, 0, 1) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot start roof closing" << sendLog;
		return -1;
	}
	return 0;
}


long
IEEC::isClosed ()
{
}


int
IEEC::endClose ()
{
	return 0;
}


IEEC::IEEC (int argc, char **argv): Dome (argc, argv)
{
	comediFile = "/dev/comedi0";

	addOption ('c', NULL, 1, "path to comedi device");
}


IEEC::~IEEC ()
{

}


int
main (int argc, char **argv)
{
	IEEC device = IEEC (argc, argv);
	return device.run ();
}
