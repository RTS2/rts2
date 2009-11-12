/* 
 * LPNHE Paris LSST controll board (filter wheel, humidity measurements).
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

#include "sensord.h"

#include <comedilib.h>

namespace rts2sensord
{

/**
 * Class for controlling National Instruments (NI) Comedi library driven board,
 * used to control TTL filter wheel and read various attached probes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @see htpp://www.comedi.org
 */
class LPNHE: public Sensor
{
	public:
		LPNHE (int argc, char **argv);
		virtual ~LPNHE ();

	protected:
		virtual int processOption (int _opt);
		virtual int init ();
		virtual int info ();

	private:
		comedi_t *comediDevice;
		const char *comediFile;

		Rts2ValueDouble *humidity;
		Rts2ValueSelection *filter;

		Rts2ValueBool *filterHomed;
		Rts2ValueBool *filterMoving;
	
		int filterChange (int num);
};

}

using namespace rts2sensord;

int LPNHE::processOption (int _opt)
{
	switch (_opt)
	{
		case 'c':
			comediFile = optarg;
			break;
		default:
			return SensorWeather::processOption (_opt);
	}
	return 0;
}

int LPNHE::init ()
{
	int ret;
	ret = Sensor::init ();
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
	ret = comedi_dio_config (comediDevice, subdev, 0, COMEDI_OUTPUT);
	if (ret != 1)
	{
	  	logStream (MESSAGE_ERROR) << "Cannot init comedi roof - subdev " << subdev
			<< " channel 0, error " << ret << sendLog;
		return -1;
	}

	return 0;
}

int LPNHE::info ()
{
	int ret;
	uint32_t value;
	ret = comedi_dio_read (comediDevice, 3, 5, &value);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read filter homed status (subdev 3, channel 5)" << sendLog;
		return -1;
	}
	filterHomed->setValueBool (value != 0);

	ret = comedi_dio_read (comediDevice, 3, 6, &value);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read filter moving status (subdev 3, channel 6)" << sendLog;
		return -1;
	}
	filterMoving->setValueBool (value != 0);

	return Sensor::info ();
}

LPNHE::LPNHE (int argc, char **argv): Sensor (argc, argv)
{
	comediFile = "/dev/comedi0";

	createValue (humidity, "HUMIDITY", "laboratory humidity", true);
	createValue (filter, "FILTER", "selected filter wheel", true);
	filter->addSelVal ("1");
	filter->addSelVal ("2");
	filter->addSelVal ("3");
	filter->addSelVal ("4");
	filter->addSelVal ("5");
	filter->addSelVal ("6");

	createValue (filterHomed, "homed", "if filter is homed", false);
	createValue (filterMoving, "moving", "if filter is moving", false);

	addOption ('c', NULL, 1, "path to comedi device");
}

LPNHE::~LPNHE ()
{

}

int LPNHE::filterChange (int num)
{
	for (int i = 0; i < num; i++)
	{
		if (comedi_dio_write (comediDevice, 2, 0, 1) != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot set roof to off" << sendLog;
			return -1;
		}
		usleep (FILTER_PULSE);
		if (comedi_dio_write (comediDevice, 2, 0, 0) != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot switch roof pulse to on" << sendLog;
			return -1;
		}
		usleep (FILTER_PULSE);
		if (comedi_dio_write (comediDevice, 2, 0, 1) != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot switch roof pulse to off, ignoring" << sendLog;
			return 0;
		}
		usleep (ROOF_TIMEOUT);
	}
	return 0;
}

int main (int argc, char **argv)
{
	LPNHE device (argc, argv);
	return device.run ();
}
