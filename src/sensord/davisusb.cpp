/* 
 * Infrastructure for Davis UDP connection.
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

#include "davis.h"
#include "davisusb.h"
#include "utilsfunc.h"

#include <fcntl.h>
#include <termios.h>

using namespace rts2sensord;

#define FRAM_CONN_TIMEOUT    60

void DavisUsb::setWeatherTimeout (time_t wait_time, const char *msg)
{
	master->setWeatherTimeout (wait_time, msg);
}

DavisUsb::DavisUsb (const char * _weather_device, int _weather_timeout, int _conn_timeout, int _bad_weather_timeout, Davis * _master):rts2core::ConnNoSend (_master)
{
	weather_device = _weather_device;
	weather_timeout = _weather_timeout;
	conn_timeout = _conn_timeout;
	bad_weather_timeout = _bad_weather_timeout;
	master = _master;
}

int DavisUsb::init ()
{
	struct termios oldtio, newtio;
        fdser = open(weather_device, O_RDWR | O_NOCTTY );

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

        if (tcsetattr (fdser, TCSAFLUSH, &newtio)) {
                logStream (MESSAGE_ERROR) << "Problem configuring serial device, check device name" << sendLog;
                return -1;
        }

        if (tcflush (fdser, TCIOFLUSH)) {
                logStream (MESSAGE_ERROR) << "Problem flushing serial device, check device name" << sendLog;
                return -1;
        }

        return 0;	
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

void DavisUsb::GetRTData(unsigned char *szData)
{
        unsigned char *tmp = szData;

	float rtOutsideTemp = ((*(tmp+14) << 8) + *(tmp+13)-320) * 5 / 90.0;

	float rtOutsideHum = (float)(*(tmp+34));

	float rtBaroCurr = ((*(tmp+9) << 8) + *(tmp+8)) * 33.865 / 1000.0;

	float rtWindDir = ((*(tmp+18) << 8) + *(tmp+17));

	float rtRainRate = ((*(tmp+43) << 8) + *(tmp+42)) * 25.4/ 100.0;

	float rtIsRaining = (rtRainRate > 0);

	avgWindSpeed = (*(tmp+16) *1.609)+0.5;

	if (rtRainRate != 0)
        {
                rain = 1;
	}
	else
	{
		rain = 0;
	}

	master->setTemperature (rtOutsideTemp);
        master->setRainRate (rtRainRate);
        master->setRain (rain);
	master->setHumidity (rtOutsideHum);
	master->setAvgWindSpeed (avgWindSpeed);
        master->setWindDir (rtWindDir);
       	master->setBaroCurr (rtBaroCurr);
        master->updateInfoTime ();

	time (&lastWeatherStatus);
                
	if (rain != 0)
	{
		time (&lastBadWeather);
		setWeatherTimeout (bad_weather_timeout, "raining");
        }
        
	master->infoAll ();
}

int DavisUsb::receive (fd_set * set)
{
	static char usbBuffer[4200];
        char ch;
        strcpy (usbBuffer, "LOOP 1\n");
        while (ReadNextChar(fdser, &ch));
        write (fdser, &usbBuffer, strlen(usbBuffer));
        tcdrain(fdser);
        int ret = ReadToBuffer(fdser, usbBuffer, sizeof(usbBuffer));

	logStream (MESSAGE_DEBUG) << "ret: " << ret << sendLog;	

        GetRTData((unsigned char *)usbBuffer);

	return 100;

}
