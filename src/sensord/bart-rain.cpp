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
		int rain_port;
		const char *rain_detector;

		rts2core::ValueBool *rain;
		rts2core::ValueInteger *timeoutRain;

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
	int flags;
	rain_port = open (rain_detector, O_RDWR | O_NOCTTY);
	if (rain_port == -1)
	{
		logStream (MESSAGE_ERROR) << "Bart::init cannot open " << rain_detector << " " << strerror (errno) << sendLog;
		return -1;
	}
	ret = ioctl (rain_port, TIOCMGET, &flags);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Bart::init cannot get flags: " << strerror (errno) << sendLog;
		return -1;
	}
	flags &= ~TIOCM_DTR;
	flags |= TIOCM_RTS;
	ret = ioctl (rain_port, TIOCMSET, &flags);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Bart::init cannot set flags: " << strerror (errno) << sendLog;
		return -1;
	}
	return 0;
}

int BartRain::info ()
{
	int flags;
	int ret;
	ret = ioctl (rain_port, TIOCMGET, &flags);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Bart::isGoodWeather flags: " << flags << " rain: " << (flags & TIOCM_RI) << sendLog;
	#endif
		// ioctl failed or it's raining..
	if (ret || !(flags & TIOCM_RI))
	{
		setWeatherTimeout (timeoutRain->getValueInteger (), "raining");
		if (!rain->getValueBool ())
		{
			rain->setValueBool (true);
			maskState (WR_RAIN, WR_RAIN, "rain detected from rain sensor");
			setIdleInfoInterval (60);
		}
	}
	else
	{
		if (rain->getValueBool ())
		{
			rain->setValueBool (false);
			maskState (WR_RAIN, 0, "no rain");
			setIdleInfoInterval (1);
		}
	}
	return SensorWeather::info ();
}

BartRain::BartRain (int argc, char **argv):SensorWeather (argc, argv)
{
	rain_detector = "/dev/ttyS0";
	rain_port = -1;

	createValue (rain, "rain", "true if sensor detects rain", false);
	rain->setValueBool (false);

	createValue (timeoutRain, "timeout_rain", "rain timeout in seconds", false, RTS2_VALUE_WRITABLE);
	timeoutRain->setValueInteger (3600);

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
