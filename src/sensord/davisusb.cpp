/* 
 * Davis weather sensor.
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

#include "davisusb.h"

#define OPT_AVG_WINDSPEED           OPT_LOCAL + 1002
#define OPT_PEEK_WINDSPEED          OPT_LOCAL + 1003

#define EVENT_DAVIS_TIMER       RTS2_LOCAL_EVENT + 666

using namespace rts2sensord;

int DavisUsb::processOption (int _opt)
{
	switch (_opt)
	{
		case 'b':
			if (cloud_bad == NULL)
				createValue (cloud_bad, "cloud_bad", "bad cloud trigger", false, RTS2_VALUE_WRITABLE);
			cloud_bad->setValueCharArr (optarg);
			break;
		case 'f':
			device_file = optarg;
			break;
		case OPT_PEEK_WINDSPEED:
			maxPeekWindSpeed->setValueCharArr (optarg);
			break;
		case OPT_AVG_WINDSPEED:
			maxWindSpeed->setValueCharArr (optarg);
			break;
		default:
			return SensorWeather::processOption (_opt);
	}
	return 0;
}

int DavisUsb::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;	
	}

	struct termios oldtio, newtio;
        fdser = open (device_file, O_RDWR | O_NOCTTY );

        tcgetattr(fdser, &oldtio);
        bzero(&newtio, sizeof(newtio));
        newtio.c_cflag = CRTSCTS | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNBRK | IGNPAR;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        newtio.c_cc[VTIME]    = 10;
        newtio.c_cc[VMIN]     = 0;
        cfsetospeed (&newtio, B19200);
        cfsetispeed (&newtio, B19200);

        if (tcsetattr (fdser, TCSAFLUSH, &newtio)) 
	{
                logStream (MESSAGE_ERROR) << "Problem configuring serial device, check device name" << sendLog;
                return -1;
        }

        if (tcflush (fdser, TCIOFLUSH)) 
	{
                logStream (MESSAGE_ERROR) << "Problem flushing serial device, check device name" << sendLog;
                return -1;
        }	

	return 0;
}

void DavisUsb::postEvent (rts2core::Event *event)
{
        if (event->getType () == EVENT_DAVIS_TIMER)
        {
                getAndParseData ();
                addTimer (2, event);
                return;
        }
        SensorWeather::postEvent (event);
}

int DavisUsb::ReadNextChar (int device, char *ch)
{
        int ret = read (device, ch, 1);

        if (ret == -1)
                logStream (MESSAGE_ERROR) << "Problem reading Davis device" << sendLog;

        return ret;
}

int DavisUsb::ReadToBuffer (int device, char *buffer, int bsize)
{
        int pos = 0;
        char *tmp = buffer;

        while(pos < bsize) {
                if(!ReadNextChar (device, tmp++))
                        return pos;
                ++pos;
        }
        return -1;
}

int DavisUsb::idle ()
{
	if (getLastInfoTime () > connTimeout->getValueInteger ())
	{
		if (isGoodWeather ())
		{
			logStream (MESSAGE_ERROR) << "Weather station did not send any data for " << connTimeout->getValueInteger () << " seconds, switching to bad weather" << sendLog;
		}
		std::ostringstream os;
		os << "cannot retrieve information from Davis sensor within last " << connTimeout->getValueInteger () << " seconds";
		setWeatherTimeout (300, os.str ().c_str ());
	}
	return SensorWeather::idle ();
}

DavisUsb::DavisUsb (int argc, char **argv):SensorWeather (argc, argv, 180)
{
  	createValue (connTimeout, "conn_timeout", "connection timeout", false, RTS2_VALUE_WRITABLE);
	connTimeout->setValueInteger (360);

	createValue (temperature, "DOME_TMP", "temperature in degrees C", true);
	createValue (humidity, "DOME_HUM", "(outside) humidity", true);
	createValue (rain, "RAIN", "whenever is raining", true);
	rain->setValueInteger (true);

	createValue (avgWindSpeed, "AVGWIND", "average windspeed", true);
	createValue (avgWindSpeedStat, "AVGWINDS", "average windspeed statistic", false);
	createValue (peekWindSpeed, "PEEKWIND", "peek windspeed", true);
	createValue (windDir, "WINDDIR", "wind direction", true, RTS2_DT_DEGREES);

	createValue (rainRate, "rain_rate", "rain rate from bucket sensor", false);

	createValue (pressure, "BAR_PRESS", "barometric pressure", true);
        
	createValue (maxWindSpeed, "max_windspeed", "maximal average windspeed", false, RTS2_VALUE_WRITABLE);
	createValue (maxPeekWindSpeed, "max_peek_windspeed", "maximal peek windspeed", false, RTS2_VALUE_WRITABLE);

	maxWindSpeed->setValueFloat (NAN);
	maxPeekWindSpeed->setValueFloat (NAN);

	createValue (maxHumidity, "max_humidity", "maximal allowed humidity", false, RTS2_VALUE_WRITABLE);
	maxHumidity->setValueFloat (NAN);

	weatherConn = NULL;

	wetness = NULL;

	cloud = NULL;
	cloudTop = NULL;
	cloudBottom = NULL;
	cloud_bad = NULL;

	
	addOption (OPT_PEEK_WINDSPEED, "max-peek-windspeed", 1, "maximal allowed peek windspeed (in m/sec)");
	addOption (OPT_AVG_WINDSPEED, "max-avg-windspeed", 1, "maximal allowed average windspeed (in m/sec");

	device_file = NULL;

	addOption ('f', NULL, 1, "USB/serial port on which is Davis meteo station connected");
}

int DavisUsb::info ()
{

	// nothing 

	return 0;
}

int DavisUsb::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == connTimeout)
	{
		weatherConn->setConnTimeout (new_value->getValueInteger ());
		return 0;
	}
	return SensorWeather::setValue (old_value, new_value);
}

void DavisUsb::setWetness (double _wetness)
{
      if (wetness == NULL)
      {
	      createValue (wetness, "WETNESS", "wetness index");
	      updateMetaInformations (wetness);
      }
      wetness->setValueDouble (_wetness);
}

void DavisUsb::setCloud (double _cloud, double _top, double _bottom)
{
      if (cloud == NULL)
      {
	      createValue (cloud, "CLOUD_S", "cloud sensor value", true);
	      updateMetaInformations (cloud);
	      createValue (cloudTop, "CLOUD_T", "cloud sensor top temperature", true);
	      updateMetaInformations (cloudTop);
	      createValue (cloudBottom, "CLOUD_B", "cloud sensor bottom temperature", true);
	      updateMetaInformations (cloudBottom);
      }

      cloud->setValueDouble (_cloud);
      cloudTop->setValueDouble (_top);
      cloudBottom->setValueDouble (_bottom);
      if (cloud_bad != NULL && cloud->getValueFloat () <= cloud_bad->getValueFloat ())
      {
	      setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT, "cloud sensor reports cloudy");
	      valueError (cloud);
      }
      else
      {
              valueGood (cloud);
      }
}

int DavisUsb::getAndParseData ()
{
	static char buf[4200];
        char ch;
        strcpy (buf, "LOOP 1\n");
        while (ReadNextChar(fdser, &ch));
        write (fdser, &buf, strlen(buf));
        tcdrain(fdser);
        int ret = ReadToBuffer(fdser, buf, sizeof(buf));

        memcpy((unsigned char*)&rcd, buf, sizeof(RTDATA));

        logStream (MESSAGE_DEBUG) << "test ( " << ret <<  " ): " << rcd.wBarometer * 33.865 / 1000.0 << sendLog;

        return 0;
}

int main (int argc, char **argv)
{
	DavisUsb device = DavisUsb (argc, argv);
	return device.run ();
}
