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

#define FILTER_PULSE  USEC_SEC / 5

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

		virtual int info ();

		virtual int commandAuthorized (Rts2Conn *conn);

	protected:
		virtual int processOption (int _opt);
		virtual int init ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

	private:
		comedi_t *comediDevice;
		const char *comediFile;

		rts2core::ValueDouble *humidity;
		rts2core::ValueDouble *vacuum;
		rts2core::ValueSelection *filter;

		rts2core::ValueBool *filterHomed;
		rts2core::ValueBool *filterMoving;
		rts2core::ValueBool *shutter;

		int filterChange (int num);

		int homeFilter ();

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
			return Sensor::processOption (_opt);
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

	int subtype = comedi_get_subdevice_type (comediDevice, 2);
	if (subtype != COMEDI_SUBD_DIO)
	{
		logStream (MESSAGE_ERROR) << "Subdevice 2 is not a digital I/O device" << sendLog;
		return -1;
	}

	/* input port - sensors - subdevice 3 */
	int subdev = 2;
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
	sleep (2);
	/* output port - roof - subdevice 2 */
	subdev = 2;
	ret = comedi_dio_config (comediDevice, subdev, 0, COMEDI_OUTPUT);
	if (ret != 1)
	{
	  	logStream (MESSAGE_ERROR) << "Cannot init comedi roof - subdev " << subdev
			<< " channel 0, error " << ret << sendLog;
		return -1;
	}
	ret = comedi_dio_config (comediDevice, subdev, 2, COMEDI_OUTPUT);
	if (ret != 1)
	{
	  	logStream (MESSAGE_ERROR) << "Cannot init comedi roof - subdev " << subdev
			<< " channel 2, error " << ret << sendLog;
		return -1;
	}

	return homeFilter ();
}

int LPNHE::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == filter)
	{
		int c = newValue->getValueInteger () - oldValue->getValueInteger ();
		if (c < 0)
			c += 6;
		return filterChange (c) == 0 ? 0 : -2;
	}
	if (oldValue == shutter)
	{
		return comedi_dio_write (comediDevice, 2, 2, ((rts2core::ValueBool *)newValue)->getValueBool ()) == 1 ? 0 : -2;
	}
	return Sensor::setValue (oldValue, newValue);
}

int LPNHE::info ()
{
	int ret;
	uint32_t value;
	ret = comedi_dio_read (comediDevice, 2, 5, &value);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read filter homed status (subdev 3, channel 5)" << sendLog;
		return -1;
	}
	filterHomed->setValueBool (value == 0);
	if (filterHomed->getValueBool () == true)
		filter->setValueInteger (0);

	ret = comedi_dio_read (comediDevice, 2, 6, &value);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read filter moving status (subdev 3, channel 6)" << sendLog;
		return -1;
	}
	filterMoving->setValueBool (value != 0);

	double val;
	ret = getVolts (0, 4, val);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot read out vacuum level" << sendLog;
		return -1;
	}
	vacuum->setValueDouble (val);

	return Sensor::info ();
}

int LPNHE::commandAuthorized (Rts2Conn *conn)
{
	int ret;
	if (conn->isCommand ("home"))
	{
		ret = homeFilter ();
		if (ret)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot home filter");
		return ret;
	}

	return Sensor::commandAuthorized (conn);
}

LPNHE::LPNHE (int argc, char **argv): Sensor (argc, argv)
{
	comediFile = "/dev/comedi0";

	createValue (humidity, "HUMIDITY", "laboratory humidity", true);
	createValue (vacuum, "VACUUM", "Dewar vacuum level", true);
	createValue (filter, "FILTER", "selected filter wheel", true, RTS2_VALUE_WRITABLE);
	filter->addSelVal ("1");
	filter->addSelVal ("2");
	filter->addSelVal ("3");
	filter->addSelVal ("4");
	filter->addSelVal ("5");
	filter->addSelVal ("6");

	createValue (filterHomed, "homed", "if filter is homed", false);
	createValue (filterMoving, "moving", "if filter is moving", false);
	createValue (shutter, "shutter", "shutter opened", true, RTS2_VALUE_WRITABLE);
	shutter->setValueBool (false);

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
		}

		// wait for moving becoming low
		uint32_t value;
		double endTime = getNow () + 20;
		do
		{
			if (comedi_dio_read (comediDevice, 2, 6, &value) != 1)
			{
				logStream (MESSAGE_ERROR) << "Cannot check for filter moving status (subdevice 2, channel 6) during moving" << sendLog;
				return -1;
			}
			usleep (USEC_SEC / 10);
		}
		while (value != 0 && getNow () < endTime);

		if (value != 0)
		{
			logStream (MESSAGE_ERROR) << "Timeout occured during filter movement" << sendLog;
			return -1;
		}
	}
	return 0;
}

int LPNHE::homeFilter ()
{
	for (int i = 8; i > 0; i--)
	{
		uint32_t value;
		if (comedi_dio_read (comediDevice, 2, 5, &value) != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot read homed switch value (subdevice 2, channel 5) during filter homing" << sendLog;
			return -1;
		}
		if (value == 0)
		{
			filter->setValueInteger (0);
			sendValueAll (filter);
			return 0;
		}
		if (filterChange (1))
			return -1;
	}
	logStream (MESSAGE_ERROR) << "Cannot find homing position" << sendLog;
	return -1;
}

int LPNHE::getVolts (int subdevice, int channel, double &volts)
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

int main (int argc, char **argv)
{
	LPNHE device = LPNHE (argc, argv);
	return device.run ();
}
