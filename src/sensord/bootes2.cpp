/* 
 * Access data from Bootes 2 weather station.
 * Copyright (C) 2009,2011 Petr Kubanek <petr@kubanek.net>
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

// top read all informations from temperature sensor
#define LIFOSIZE          60

#define OPT_HUMI_BAD      OPT_LOCAL + 318
#define OPT_HUMI_GOOD     OPT_LOCAL + 319

namespace rts2sensord
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
	public:
		Bootes2 (int argc, char **argv);
		virtual ~Bootes2 ();

	protected:
		virtual int processOption (int _opt);
		virtual int initHardware ();
		virtual int info ();

		virtual void valueChanged (rts2core::Value *v);

	private:
		comedi_t *comediDevice;
		const char *comediFile;

		rts2core::ValueBool *raining;
	
		rts2core::ValueDoubleStat *tempMeas;
		rts2core::ValueDoubleStat *humiMeas;
		
		rts2core::ValueDouble *humBad;
		rts2core::ValueDouble *humGood;

		rts2core::DoubleArray *vd;
		rts2core::BoolArray *vb;

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
};

}

using namespace rts2sensord;

int Bootes2::getVolts (int subdevice, int channel, double &volts)
{
	int max;
	comedi_range *rqn;
	int range = 0;
	lsampl_t data;

	max = comedi_get_maxdata (comediDevice, subdevice, channel);
	if (max == 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot get max data from subdevice " << subdevice << " channel " << channel << sendLog;
		return -1;
	}
	rqn = comedi_get_range (comediDevice, subdevice, channel, range);
	if (rqn == NULL)
	{
		logStream (MESSAGE_ERROR) << "Cannot get range from subdevice " << subdevice << " channel " << channel << sendLog;
		return -1;
	}
	if (comedi_data_read (comediDevice, subdevice, channel, range, AREF_GROUND, &data) != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read data from subdevice " << subdevice << " channel " << channel << sendLog;
		return -1;
	}
	volts = comedi_to_phys (data, rqn, max);
	if (isnan (volts))
	{
		logStream (MESSAGE_ERROR) << "Cannot convert data from subdevice " << subdevice << " channel " << channel << " to physical units" << sendLog;
		return -1;
	}
	return 0;
}

int Bootes2::updateHumidity ()
{
	double hum;
	if (getVolts (0, 0, hum))
		return -1;
	hum *= 100;
	humiMeas->addValue (hum, LIFOSIZE);
	humiMeas->calculate ();
	return 0;
}

int Bootes2::updateTemperature ()
{
	double temp;
	if (getVolts (0, 2, temp))
		return -1;
	temp = (temp - 0.4) * 100;
	tempMeas->addValue (temp, LIFOSIZE);
	tempMeas->calculate ();
	return 0;
}

int Bootes2::processOption (int _opt)
{
	switch (_opt)
	{
		case 'c':
			comediFile = optarg;
			break;
		case OPT_HUMI_BAD:
			humBad->setValueCharArr (optarg);
			break;
		case OPT_HUMI_GOOD:
			humGood->setValueCharArr (optarg);
			break;
		default:
			return SensorWeather::processOption (_opt);
	}
	return 0;
}

int Bootes2::initHardware ()
{
	int ret;

	comediDevice = comedi_open (comediFile);
	if (comediDevice == NULL)
	{
		logStream (MESSAGE_ERROR) << "Cannot open comedi port " << comediFile << sendLog;
		return -1;
	}

	int subdev = 3;
	int i;
	/* input port - sensors - subdevice 3 */
	for (i = 0; i < 8; i++)
	{
		ret = comedi_dio_config (comediDevice, subdev, i, COMEDI_INPUT);
		if (ret != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot init comedi sensors - subdev " << subdev << " channel " << i << ", error " << ret << sendLog;
			return -1;
		}
	}
	/* output port - roof - subdevice 2 */
	subdev = 2;
	for (i = 0; i < 8; i++)
	{
		ret = comedi_dio_config (comediDevice, subdev, i, COMEDI_OUTPUT);
		if (ret != 1)
		{
		  	logStream (MESSAGE_ERROR) << "Cannot init comedi roof - subdev " << subdev << " channel " << i << ", error " << ret << sendLog;
			return -1;
		}
		vb->addValue (false);
	}

	return 0;
}

int Bootes2::info ()
{
	int ret;
	uint32_t value;
	ret = comedi_dio_read (comediDevice, 3, 5, &value);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read rain status (subdev 3, channel 5)" << sendLog;
		setWeatherTimeout (3600, "cannot read rain status");
		return -1;
	}
	if (value == 0)
	{
		setWeatherTimeout (3600, "raining");
		if (raining->getValueBool () == false)
			logStream (MESSAGE_INFO) << "raining, switching to bad weather" << sendLog;
		raining->setValueBool (true);
	}
	else
	{
		if (raining->getValueBool () == true)
			logStream (MESSAGE_INFO) << "rains ends" << sendLog;
		raining->setValueBool (false);
	}
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
	if (!isnan (humBad->getValueDouble ()) && humiMeas->getValueDouble () > humBad->getValueDouble ())
	{
		setWeatherTimeout (600, "humidity rised above humidity_bad");
	}
	if (!isnan (humGood->getValueDouble ()) && humiMeas->getValueDouble () > humGood->getValueDouble () && getWeatherState () == false)
	{
		setWeatherTimeout (600, "humidity does not drop bellow humidity_good");
	}

	std::vector <double> vals;

	for (int i = 0; i < 8; i++)
	{
		double v;
		getVolts (0, i, v);
		vals.push_back (v);
	}

	vd->setValueArray (vals);

	return SensorWeather::info ();
}

void Bootes2::valueChanged (rts2core::Value *v)
{
	if (v == vb)
	{
		for (int i = 0; i < 8; i++)
		{
			comedi_dio_write (comediDevice, 2, i, (*vb)[i]);
		}
	}
}

Bootes2::Bootes2 (int argc, char **argv): SensorWeather (argc, argv)
{
	comediFile = "/dev/comedi0";

	createValue (raining, "raining", "if it is raining (from rain detector)", false);

	createValue (tempMeas, "TEMP", "outside temperature", true);
	createValue (humiMeas, "HUMIDITY", "[%] outside humidity", true);

	createValue (humBad, "humidity_bad", "[%] when humidity is above this value, weather is bad", false, RTS2_VALUE_WRITABLE);
	createValue (humGood, "humidity_good", "[%] when humidity is bellow this value, weather is good", false, RTS2_VALUE_WRITABLE);

	createValue (vd, "values", "[V] values", false);
	createValue (vb, "vb", "LEDs", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	addOption ('c', NULL, 1, "path to comedi device");
	addOption (OPT_HUMI_BAD, "humidity_bad", 1, "[%] when humidity is above this value, weather is bad");
	addOption (OPT_HUMI_GOOD, "humidity_good", 1, "[%] when humidity is bellow this value, weather is good");

	setIdleInfoInterval (5);
}

Bootes2::~Bootes2 ()
{

}

int main (int argc, char **argv)
{
	Bootes2 device = Bootes2 (argc, argv);
	return device.run ();
}
