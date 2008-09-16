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

// top read all informations from temperature sensor
#define LIFOSIZE	60

namespace rts2dome
{

/**
 * Class for control of the Bootes telescope roof. This class uses some PCI
 * board which is accesible from Comedi library.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @see htpp://www.comedi.org
 */
class Bootes2: public Rts2DevDome
{
	private:
		comedi_t *comediDevice;
		const char *comediFile;


		Rts2ValueDoubleStat *tempMeas;
		Rts2ValueDoubleStat *humiMeas;

		// comedi status
		// abrierta 1 - open 1
		int A1;
		// open 2
		int A2;
		// cerrada 1 - close 1
		int C1;
		// close 2
		int C2;
		int RAIN1;
		int RAIN2;

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

	protected:
		virtual int processOption (int _opt);
		virtual int init ();
		virtual int info ();

	public:
		Bootes2 (int argc, char **argv);
		virtual ~Bootes2 ();
};

}

using namespace rts2dome;


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
Bootes2::processOption (int _opt)
{
	switch (_opt)
	{
		case 'c':
			comediFile = optarg;
			break;
		default:
			return Rts2DevDome::processOption (_opt);
	}
	return 0;
}


int
Bootes2::init ()
{
	int ret;
	ret = Rts2DevDome::init ();
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

	return Rts2DevDome::info ();
}


Bootes2::Bootes2 (int argc, char **argv): Rts2DevDome (argc, argv)
{
	comediFile = "/dev/comedi0";

	createValue (tempMeas, "TEMP", "outside temperature");
	createValue (humiMeas, "HUMIDITY", "outside humidity");

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
