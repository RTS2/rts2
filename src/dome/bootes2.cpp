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
#define LIFOSIZE          60

#define RAIN_TIMEOUT    3600

#define ROOF_PULSE	100000
#define ROOF_TIMEOUT	5000000


class ComediDOError
{
	private:
		const char *name;
	public:
		ComediDOError (const char *_name)
		{
			name = _name;
		}

		friend std::ostream & operator << (std::ostream & os, ComediDOError & err);
};

std::ostream & operator << (std::ostream & os, ComediDOError & err)
{
	os << "Cannot read digital input " << err.name;
	return os;
}

namespace rts2dome
{

/**
 * Class for control of the Bootes telescope roof. This class uses some PCI
 * board which is accesible from Comedi library.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @see htpp://www.comedi.org
 */
class Bootes2: public Dome
{
	private:
		comedi_t *comediDevice;
		const char *comediFile;

		rts2core::ValueDoubleStat *tempMeas;
		rts2core::ValueDoubleStat *humiMeas;

		rts2core::ValueBool *swOpenLeft;
		rts2core::ValueBool *swCloseLeft;
		rts2core::ValueBool *swOpenRight;
		rts2core::ValueBool *swCloseRight;

		rts2core::ValueBool *raining;

		void comediReadDIO (int channel, rts2core::ValueBool *val, const char *name);

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

		/**
		 * Pull on roof trigger.
		 *
		 * @return -1 on failure, 0 on success.
		 */
		int roofChange ();

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

		virtual bool isGoodWeather ();

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
	if (std::isnan (volts))
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

void Bootes2::comediReadDIO (int channel, rts2core::ValueBool *val, const char *name)
{
	int ret;
	uint32_t v;
	ret = comedi_dio_read (comediDevice, 3, channel, &v);
	if (ret != 1)
		throw ComediDOError (name);
	val->setValueBool (v != 0);
}

int Bootes2::updateStatus ()
{
	swOpenLeft->setValueBool (true);
	swOpenRight->setValueBool (true);
	swCloseLeft->setValueBool (false);
	swCloseRight->setValueBool (false);
	try
	{
		comediReadDIO (2, swCloseLeft, "close left");
		comediReadDIO (3, swCloseRight, "close right");

		comediReadDIO (0, swOpenLeft, "open left");
		comediReadDIO (1, swOpenRight, "open right");
	}
	catch (ComediDOError &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
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
	ret = updateStatus ();
	if (ret)
		return -1;

	if (swCloseLeft->getValueBool () && swCloseRight->getValueBool ())
		maskState (DOME_DOME_MASK, DOME_CLOSED, "Init: dome is closed");
	else if (swOpenLeft->getValueBool () && swOpenRight->getValueBool ())
	  	maskState (DOME_DOME_MASK, DOME_OPENED, "Init: dome is opened");

	return Dome::info ();
}


int
Bootes2::startOpen ()
{
	if (!isGoodWeather ())
		return -1;

	return roofChange ();
}


long
Bootes2::isOpened ()
{
	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;
	if (swOpenLeft->getValueBool () && swOpenRight->getValueBool ())
		return USEC_SEC;
	return -2;
}


int
Bootes2::endOpen ()
{
	return 0;
}


int
Bootes2::startClose ()
{
	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;

	if (swCloseLeft->getValueBool () && swCloseRight->getValueBool ())
		return -1;

	return roofChange ();
}


long
Bootes2::isClosed ()
{
	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;
	if (swCloseLeft->getValueBool () && swCloseRight->getValueBool ())
		return USEC_SEC;
	return -2;
}


int
Bootes2::endClose ()
{
	return 0;
}


bool
Bootes2::isGoodWeather ()
{
	if (getIgnoreMeteo () == true)
		return true;
	int ret = updateStatus ();
	if (ret)
		return false;
	// if it's raining
	if (raining->getValueBool () == true)
		return false;
	return Dome::isGoodWeather ();
}


Bootes2::Bootes2 (int argc, char **argv): Dome (argc, argv)
{
	comediFile = "/dev/comedi0";

	createValue (tempMeas, "TEMP", "outside temperature", true);
	createValue (humiMeas, "HUMIDITY", "outside humidity", true);

	createValue (swOpenLeft, "sw_open_left", "left open end switch", false);
	createValue (swCloseLeft, "sw_close_left", "left close end switch", false);
	createValue (swOpenRight, "sw_open_right", "right open end swith", false);
	createValue (swCloseRight, "sw_close_right", "right close end switch", false);

	createValue (raining, "RAIN", "if it's raining", true);

	raining->setValueBool (false);

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
