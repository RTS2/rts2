/* 
 * Vantage Davis USB/serial driver.
 * Copyright (C) 2015 Stanislav Vitek 
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#include "connection/serial.h"

using namespace rts2sensord;

void DavisUsb::setWeatherTimeout (time_t wait_time, const char *msg)
{
	master->setWeatherTimeout (wait_time, msg);
}

DavisUsb::DavisUsb (const char * _weather_device, int _weather_timeout, int _conn_timeout, int _bad_weather_timeout, Davis * _master):rts2core::ConnSerial (_weather_device, _master, BS19200, C8, NONE, 100)
{
	weather_timeout = _weather_timeout;
	bad_weather_timeout = _bad_weather_timeout;
	master = _master;
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
