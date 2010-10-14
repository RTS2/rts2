/* 
 * Driver for Aerotech asix controller.
 * Copyright (C) 2010 Petr Kubanek <kubanek@fzu.cz> Institute of Physics, Czech Republic
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

#include <fstream>
#include <sstream>

using namespace rts2sensord;

class FlwoWeather:public SensorWeather
{
	public:
		FlwoWeather (int argc, char **argv);

	protected:
		virtual int processOption (int opt);
		/**
		 * Retrieves weather informations.
		 */
		virtual int info ();

		virtual bool isGoodWeather ();

	private:
		const char *weatherFile;

		Rts2ValueFloat *outsideTemp;
		Rts2ValueFloat *windSpeed;
		Rts2ValueFloat *windGustSpeed;
		Rts2ValueFloat *windDir;
		Rts2ValueFloat *pressure;
		Rts2ValueFloat *humidity;
		Rts2ValueFloat *rain;
		Rts2ValueFloat *dewpoint;
		Rts2ValueBool *hatRain;
};

FlwoWeather::FlwoWeather (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (outsideTemp, "outside_temp", "[C] outside temperature", false);
	createValue (windSpeed, "wind_speed", "[mph] windspeed", false);
	createValue (windGustSpeed, "wind_gust_speed", "[mph] wind gust speed", false);
	createValue (windDir, "wind_direction", "[deg] wind direction", false);
	createValue (pressure, "pressure", "[mB] atmospheric pressure", false);
	createValue (humidity, "humidity", "[%] outside humidity", false);
	createValue (rain, "rain", "[inch] total accumulated rain", false);
	createValue (dewpoint, "dewpoint", "[C] dewpoint", false);
	createValue (hatRain, "HAT_rain", "hat rain status", false);

	weatherFile = NULL;

	addOption ('f', NULL, 1, "file with weather informations");
}

int FlwoWeather::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			weatherFile = optarg;
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int FlwoWeather::info ()
{
  	std::ifstream ifile (weatherFile, std::ifstream::in);
	time_t et = 0;
	int processed = 0;
	while (ifile.good ())
	{
		char line[500];
		ifile.getline (line, 500);
		// date..
		if (line[0] == '(')
		{
		  	struct tm tm;
			char sep;
			std::istringstream is (line);
			is >> sep >> tm.tm_year >> sep >> tm.tm_mon >> sep >> tm.tm_mday >> sep >> tm.tm_hour >> sep >> tm.tm_min >> sep >> tm.tm_sec;
			if (is.good ())
			{
			  	tm.tm_year -= 1900;
				tm.tm_mon--;
				std::string old_tz;
				char p_tz[100];
				if (getenv("TZ"))
					old_tz = std::string (getenv ("TZ"));

				putenv ((char*) "TZ=UTC");

			  	et = mktime (&tm);

				strcpy (p_tz, "TZ=");

				if (old_tz.length () > 0)
				{
					strncat (p_tz, old_tz.c_str (), 96);
				}
				putenv (p_tz);
			}
		}
		else if (strstr (line, "self.states['") == line)
		{
			// parse fields..
			char *ch = strchr (line + 13, '\'');
			if (ch == NULL)
			  	continue;
			*ch = '\0';
			ch++;
			if (*ch != ']')
			  	continue;
			ch++;
			// find value
			char *name = line + 13;
			if (strstr (name, "outside_temp") == name)
			{
			  	outsideTemp->setValueCharArr (ch);
				processed |= 1;
			}
			else if (strstr (name, "wind_speed") == name)
			{
			  	windSpeed->setValueCharArr (ch);
				processed |= 1 << 1;
			}
			else if (strstr (name, "wind_gust_speed") == name)
			{
			  	windGustSpeed->setValueCharArr (ch);
				processed |= 1 << 2;
			}
			else if (strstr (name, "wind_direction") == name)
			{
			  	windDir->setValueCharArr (ch);
				processed |= 1 << 3;
			}
			else if (strstr (name, "barometer") == name)
			{
			  	pressure->setValueCharArr (ch);
			  	processed |= 1 << 4;
			}
			else if (strstr (name, "outside_humidity") == name)
			{
			  	humidity->setValueCharArr (ch);
				if (humidity->getValueFloat () > 93)
				  	setWeatherTimeout (600, "humidity is above 93%");
				processed |= 1 << 5;
			}
			else if (strstr (name, "total_rain") == name)
			{
			  	rain->setValueCharArr (ch);
				processed |= 1 << 6;
			}
			else if (strstr (name, "dewpoint") == name)
			{
			  	dewpoint->setValueCharArr (ch);
				processed |= 1 << 7;
			}
			else if (strstr (name, "hat6_rain") == name)
			{
				if (strstr (ch, "clear") == ch)
				  	hatRain->setValueBool (false);
				else
				{
				  	hatRain->setValueBool (true);	
					setWeatherTimeout (600, "hat6 is reporting rain");
				}
				processed |= 1 << 8;	
			}
			else if (strstr (name, "sample_date") != name)
			{
			  	logStream (MESSAGE_ERROR) << "invalid line from " << weatherFile << ": " << line << sendLog;
			  	et = 0;
			}
		}
		else if (line[0] != '\0')
		{
		  	logStream (MESSAGE_ERROR) << "invalid line from " << weatherFile << ": " << line << sendLog;
			et = 0;
		}
	}

	ifile.close ();

	if (et > 0 && processed == 0x1ff)
		setInfoTime (et);

	return 0;
}

bool FlwoWeather::isGoodWeather ()
{
	if (getLastInfoTime () > 60)
  	{
	  	setWeatherTimeout (30, "weather data not recived");
		return false;
	}
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	FlwoWeather device (argc, argv);
	return device.run ();
}
