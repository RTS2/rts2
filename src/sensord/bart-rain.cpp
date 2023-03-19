/* 
 * Bart rain sensor.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

//#define DEBUG_EXTRA

#include "sensord.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace rts2sensord
{
  
/**
 * Rain sensor used at Bart dome. It has a simple serial port connector,
 * and reports rain-no rain conditions.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class BartRain: public SensorWeather
{
	private:
		int ioctlSetRead (int flagsSet, int * flagsRead);

		int rain_port;
		int flags0, flags1;
		const char *rain_detector;

		rts2core::ValueBool *rain;
		rts2core::ValueInteger *timeoutRain;
		rts2core::ValueInteger *timeoutTechnicalProblems;

	protected:
		virtual int processOption (int _opt);
		virtual int init ();
		virtual int info ();

	public:
		BartRain (int argc, char **argv);
		virtual ~BartRain ();

};

}

using namespace rts2sensord;

int BartRain::processOption (int _opt)
{
	switch (_opt)
	{
		case 'f':
			rain_detector = optarg;
			break;
		case 'r':
			timeoutRain->setValueInteger (atoi (optarg));
			break;
		default:
			return SensorWeather::processOption (_opt);
	}
	return 0;
}

int BartRain::init ()
{
 	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;
	// init rain detector
	rain_port = open (rain_detector, O_RDWR | O_NOCTTY);
	if (rain_port == -1)
	{
		logStream (MESSAGE_ERROR) << "BartRain::init cannot open " << rain_detector << " " << strerror (errno) << sendLog;
		return -1;
	}
	ret = ioctl (rain_port, TIOCMGET, &flags0);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "BartRain::init cannot get flags: " << strerror (errno) << sendLog;
		return -1;
	}
	flags0 &= ~TIOCM_DTR;
	flags0 |= TIOCM_RTS;
	ret = ioctl (rain_port, TIOCMSET, &flags0);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "BartRain::init cannot set flags: " << strerror (errno) << sendLog;
		return -1;
	}

	// flags1 are same but reversed logic of DTR/RTS, for testing if the rain sensor is really connected
	flags1 = flags0 | TIOCM_DTR;
	flags1 &= ~TIOCM_RTS;
	return 0;
}

int BartRain::info ()
{
	int flags_actual;
	int ret;

	// first, try the "standard" setup, DTR off, RTS on
	ret = ioctlSetRead (flags0, &flags_actual);
	if (!ret)
	{
		if (flags_actual & TIOCM_RI)	// it's raining (this also implies the device is really connected!)
		{
			setWeatherTimeout (timeoutRain->getValueInteger (), "raining");
			if (!rain->getValueBool ())
			{
				rain->setValueBool (true);
				maskState (WR_RAIN, WR_RAIN, "rain detected from rain sensor");
				setIdleInfoInterval (60);
			}
		}
		else	// it isn't raining or the device is not connected (let's find out!)
		{
			// let's try the "reversed" logic, DTR on, RTS off (if the RING state is reversed as well, device is connected)
			ret = ioctlSetRead (flags1, &flags_actual);
			if(!ret)
			{
				if (flags_actual & TIOCM_RI)	// in reversed logic this means the sensor is connected and dry!
				{
					if (rain->getValueBool ())
					{
						rain->setValueBool (false);
						maskState (WR_RAIN, 0, "no rain");
						setIdleInfoInterval (1);
					}
				}
				else	// this means, the sensor device is not physically connected to the serial port
				{
					setWeatherTimeout (timeoutTechnicalProblems->getValueInteger (), "rain sensor not connected to serial port");
					if (!rain->getValueBool ())
					{
						rain->setValueBool (true);
						maskState (WR_RAIN, WR_RAIN, "rain sensor not connected to serial port");
						setIdleInfoInterval (60);
					}
				}
			}
		}
	}
	return SensorWeather::info ();
}


int BartRain::ioctlSetRead (int flagsSet, int * flagsRead)
{
	int ret;
	ret = ioctl (rain_port, TIOCMSET, &flagsSet);
	if (ret)
		logStream (MESSAGE_ERROR) << "BartRain::ioctlSetRead cannot set flags: " << strerror (errno) << sendLog;
	else
	{
		usleep (USEC_SEC / 5);
		ret = ioctl (rain_port, TIOCMGET, flagsRead);
	}

	if (ret)
	{
		setWeatherTimeout (timeoutTechnicalProblems->getValueInteger (), "serial port comm problem");
		if (!rain->getValueBool ())
		{
			rain->setValueBool (true);
			maskState (WR_RAIN, WR_RAIN, "serial port comm problem");
			setIdleInfoInterval (60);
		}
		return -1;
	}
	else
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "BartRain::ioctlSetRead flagsRead: " << *flagsRead << ", DTR: " << (*flagsRead & TIOCM_DTR) << ", RTS: " << (*flagsRead & TIOCM_RTS) << ", RING: " << (*flagsRead & TIOCM_RI) << sendLog;
		#endif
		return 0;
	}
}


BartRain::BartRain (int argc, char **argv):SensorWeather (argc, argv)
{
	rain_detector = "/dev/ttyS0";
	rain_port = -1;

	createValue (rain, "rain", "true if sensor detects rain", false);
	rain->setValueBool (false);

	createValue (timeoutRain, "timeout_rain", "rain timeout in seconds", false, RTS2_VALUE_WRITABLE);
	timeoutRain->setValueInteger (3600);

	createValue (timeoutTechnicalProblems, "timeout_technical_problems", "technical problems timeout in seconds", false, RTS2_VALUE_WRITABLE);
	timeoutTechnicalProblems->setValueInteger (600);

	addOption ('f', NULL, 1, "/dev/file for rain detector, default to /dev/ttyS0");
	addOption ('r', NULL, 1, "rain timeout in seconds. Default to 3600 seconds (= 60 minutes = 1 hour)");

	setIdleInfoInterval (1);
}

BartRain::~BartRain ()
{
	if (rain_port >= 0)
		close (rain_port);
}

int main (int argc, char **argv)
{
	BartRain device = BartRain (argc, argv);
	return device.run ();
}
