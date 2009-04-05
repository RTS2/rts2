/* 
 * Access data from Bootes 2 weather station.
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

#include "dome.h"

#include <comedilib.h>

// top read all informations from temperature sensor
#define LIFOSIZE          60

namespace rts2sensor
{

/**
 * Class for access to Bootes 2 weather station. This class uses some PCI
 * board which is accesible from Comedi library.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @see htpp://www.comedi.org
 */
class Bootes2: public SensorWeather
{
	private:
		comedi_t *comediDevice;
		const char *comediFile;

	
		Rts2ValueDoubleStat *tempMeas;
		Rts2ValueDoubleStat *humiMeas;

		/**
		 * Returns volts from the device.
		 *
		 * @param subdevice Subdevice number.
		 * @param channel   Channel number.
		 * @param volts     Returned volts.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int getVolts (int subdevice, int channel, double &volts);

		/**
		 * Update temperature measurements.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int updateTemperature ();

		/**
		 * Update humidity.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int updateHumidity ();

		/**
		 * Update status of end sensors.
		 *
		 * @return -1 on failure, 0 on success.
		 */
		int updateStatus ();

	protected:
		virtual int processOption (int _opt);
		virtual int init ();
		virtual int info ();

	public:
		Bootes2 (int argc, char **argv);
		virtual ~Bootes2 ();
};

}

using namespace rts2sensor;


int
Bootes2::getVolts (int subdevice, int channel, double &volts)
{
	int max;
	comedi_range *rqn;
	int range = 0;
	lsampl_t data;

	max = comedi_get_maxdata (comediDevice, subdevice, channel);
	if (max == 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot get max data from subdevice "
			<< subdevice << " channel " << channel << sendLog;
		return -1;
	}
	rqn = comedi_get_range (comediDevice, subdevice, channel, range);
	if (rqn == NULL)
	{
		logStream (MESSAGE_ERROR) << "Cannot get range from subdevice "
			<< subdevice << " channel " << channel << sendLog;
		return -1;
	}
	if (comedi_data_read (comediDevice, subdevice, channel, range, AREF_GROUND, &data) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read data from subdevice "
			<< subdevice << " channel " << channel << sendLog;
		return -1;
	}
	volts = comedi_to_phys (data, rqn, max);
	if (isnan (volts))
	{
		logStream (MESSAGE_ERROR) << "Cannot convert data from subdevice "
			<< subdevice << " channel " << channel << " to physical units" << sendLog;
		return -1;
	}
	return 0;
}


int
Bootes2::updateHumidity ()
{
	double hum;
	if (getVolts (0, 0, hum))
		return -1;
	hum *= 100;
	humiMeas->addValue (hum, LIFOSIZE);
	return 0;
}


int
Bootes2::updateTemperature ()
{
	double temp;
	if (getVolts (0, 2, temp))
		return -1;
	temp = (temp - 0.4) * 100;
	tempMeas->addValue (temp, LIFOSIZE);
	return 0;
}


int
Bootes2::updateStatus ()
{
	return 0;
}


int
Bootes2::roofChange ()
{
	if (comedi_dio_write (comediDevice, 2, 0, 0) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot set roof to off" << sendLog;
		return -1;
	}
	usleep (ROOF_PULSE);
	if (comedi_dio_write (comediDevice, 2, 0, 1) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot switch roof pulse to on" << sendLog;
		return -1;
	}
	usleep (ROOF_PULSE);
	if (comedi_dio_write (comediDevice, 2, 0, 0) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot switch roof pulse to off, ignoring" << sendLog;
		return 0;
	}

	usleep (ROOF_TIMEOUT);
	return 0;
}


int
Bootes2::processOption (int _opt)
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
Bootes2::init ()
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
	ret = comedi_dio_config (comediDevice, subdev, 0, COMEDI_OUTPUT);
	if (ret != 1)
	{
	  	logStream (MESSAGE_ERROR) << "Cannot init comedi roof - subdev " << subdev
			<< " channel 0, error " << ret << sendLog;
		return -1;
	}

	return 0;
}


int
Bootes2::info ()
{
	int ret;
	ret = updateTemperature ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Temperature measurement failed" << sendLog;
		return -1;
	}
	ret = updateHumidity ();
	if (ret)
	{
	 	logStream (MESSAGE_ERROR) << "Humidity measurement failed" << sendLog;
		return -1;
	}

	return SensorWeather::info ();
}


Bootes2::Bootes2 (int argc, char **argv): Dome (argc, argv)
{
	comediFile = "/dev/comedi0";

	createValue (tempMeas, "TEMP", "outside temperature", true);
	createValue (humiMeas, "HUMIDITY", "outside humidity", true);

	addOption ('c', NULL, 1, "path to comedi device");
}


Bootes2::~Bootes2 ()
{

}


int
main (int argc, char **argv)
{
	Bootes2 device = Bootes2 (argc, argv);
	return device.run ();
}
