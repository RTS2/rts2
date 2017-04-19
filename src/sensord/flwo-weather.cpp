/* 
 * Driver for FLWO weather station.
 * Copyright (C) 2010-2014 Petr Kubanek <kubanek@fzu.cz> Institute of Physics, Czech Republic
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
#include "connection/udp.h"

#include <fstream>
#include <sstream>

using namespace rts2sensord;

class FlwoWeather:public SensorWeather
{
	public:
		FlwoWeather (int argc, char **argv);
		virtual ~FlwoWeather ();
	protected:
		virtual int processOption (int opt);
		virtual int info ();

		virtual bool isGoodWeather ();

	private:
		const char *weatherFile;

		// wait values
		rts2core::ValueFloat *wait_nodata;
		rts2core::ValueFloat *wait_humidity;
		rts2core::ValueFloat *wait_wind;

		rts2core::ValueFloat *ignore_nodata;

		rts2core::ValueFloat *outsideTemp;
		rts2core::ValueDoubleStat *windSpeed;
		rts2core::ValueInteger *windSpeedAvg;
		rts2core::ValueFloat *windSpeed_limit;
		rts2core::ValueFloat *windGustSpeed;
		rts2core::ValueFloat *windGustSpeed_limit;
		rts2core::ValueFloat *windDir;
		rts2core::ValueFloat *pressure;
		rts2core::ValueFloat *humidity;
		rts2core::ValueFloat *humidity_limit;
		rts2core::ValueFloat *rain;
		rts2core::ValueFloat *totalRain;
		rts2core::ValueFloat *dewpoint;
		rts2core::ValueTime *lastPool;
};

FlwoWeather::FlwoWeather (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (wait_nodata, "wait_nodata", "[s] set bad weather when data are not refreshed after this seconds", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (wait_humidity, "wait_humidity", "[s] set bad weather when humidity is over limit for this seconds", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (wait_wind, "wait_wind", "[s] set bad weather when wind is over limit for this number of seconds", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	createValue (ignore_nodata, "ignore_nodata", "[s] no data packet will be ignored for the given period", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	wait_nodata->setValueFloat (300);
	wait_humidity->setValueFloat (300);
	wait_wind->setValueFloat (300);

	ignore_nodata->setValueInteger (300);

	createValue (outsideTemp, "outside_temp", "[C] outside temperature", false);
	createValue (windSpeed, "wind_speed", "[mph] windspeed", false);
	createValue (windSpeedAvg, "wind_speed_avg", "number of measurements to average", false, RTS2_VALUE_WRITABLE);
	windSpeedAvg->setValueInteger (8);
	createValue (windSpeed_limit, "wind_speed_limit", "[mph] windspeed limit", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	windSpeed_limit->setValueFloat (40);

	createValue (windGustSpeed, "wind_gust", "[mph] wind gust speed", false);
	createValue (windGustSpeed_limit, "wind_gust_limit", "[mph] wind gust speed limit", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	windGustSpeed_limit->setValueFloat (50);

	createValue (windDir, "wind_direction", "[deg] wind direction", false);
	createValue (pressure, "pressure", "[mB] atmospheric pressure", false);
	createValue (humidity, "humidity", "[%] outside humidity", false);
	createValue (humidity_limit, "humidity_limit", "[%] humidity limit for bad weather", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	humidity_limit->setValueFloat (90);
	createValue (rain, "rain", "[inch] Vaisala's idea of current rainfall", false);
	createValue (totalRain, "total_rain", "[inch] total accumulated rain", false);
	createValue (dewpoint, "dewpoint", "[C] dewpoint", false);
	createValue (lastPool, "last_pool", "last file pool", false);
	lastPool->setValueDouble (NAN);

	weatherFile = NULL;

	addOption ('f', NULL, 1, "file with weather informations");

	setIdleInfoInterval (15);
}

FlwoWeather::~FlwoWeather ()
{
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
		if (et == 0)
		{
		  	struct tm tm;
			bzero (&tm, sizeof (tm));
			char sep;
			et = 1; // as we read the first line..
			std::istringstream is (line);
			is >> tm.tm_year >> sep >> tm.tm_mon >> sep >> tm.tm_mday >> sep >> tm.tm_hour >> sep >> tm.tm_min >> sep >> tm.tm_sec;
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
		else
		{
			char *ch = strchr (line, '=');
			if (ch == NULL)
			  	continue;
			*ch = '\0';
			ch++;
			// find value
			if (strstr (line, "outsideTemp") == line)
			{
			  	outsideTemp->setValueCharArr (ch);
				processed |= 1;
			}
			else if (strstr (line, "windSpeed") == line)
			{
				char *endptr;
				double v = strtod (ch, &endptr);
				if (endptr != ch && *endptr == '\0')
				{
					windSpeed->addValue (v, windSpeedAvg->getValueInteger ());
					windSpeed->calculate ();
					processed |= 1 << 1;
				}
			}
			else if (strstr (line, "windGustSpeed") == line)
			{
			  	windGustSpeed->setValueCharArr (ch);
				processed |= 1 << 2;
			}
			else if (strstr (line, "windDirection") == line)
			{
			  	windDir->setValueCharArr (ch);
				processed |= 1 << 3;
			}
			else if (strstr (line, "barometer") == line)
			{
			  	pressure->setValueCharArr (ch);
			  	processed |= 1 << 4;
			}
			else if (strstr (line, "outsideHumidity") == line)
			{
			  	humidity->setValueCharArr (ch);
				processed |= 1 << 5;
			}
			else if (strstr (line, "wxt510Rain") == line)
			{
				rain->setValueCharArr (ch);
				processed |= 1 << 6;
			}
			else if (strstr (line, "totalRain") == line)
			{
			  	totalRain->setValueCharArr (ch);
				processed |= 1 << 7;
			}
			else if (strstr (line, "outsideDewPt") == line)
			{
			  	dewpoint->setValueCharArr (ch);
				processed |= 1 << 8;
			}
			else
			{
			  	logStream (MESSAGE_ERROR) << "invalid line from " << weatherFile << ": " << line << sendLog;
			  	et = 0;
			}
		}
	}

	ifile.close ();

	if (et > 0 && processed == 0x1ff)
	{
		lastPool->setValueInteger (et);
		sendValueAll (lastPool);
		processed = 0;
	}
	else
	{
		processed = -1;
	}

	setInfoTime (lastPool->getValueInteger ());

	// processed is overwritten, see above
	return processed;
}

bool FlwoWeather::isGoodWeather ()
{
  	bool ret = true;
	double lastNow = getNow () - ignore_nodata->getValueFloat ();
	if (getLastInfoTime () > ignore_nodata->getValueFloat ())
  	{
	  	std::ostringstream os;
		os << "weather data not received, Pairitel:" << Timestamp (lastPool->getValueDouble ());
		setWeatherTimeout (wait_nodata->getValueInteger (), "weather data not received");
		if (std::isnan (lastPool->getValueDouble ()) || lastPool->getValueDouble () < lastNow)
		{
			humidity->setValueFloat (NAN);
			valueError (humidity);
			sendValueAll (humidity);

			outsideTemp->setValueFloat (NAN);
			sendValueAll (outsideTemp);

			windSpeed->clearStat ();
			valueError (windSpeed);
			sendValueAll (windSpeed);

			windGustSpeed->setValueFloat (NAN);
			valueError (windGustSpeed);
			sendValueAll (windGustSpeed);

			windDir->setValueFloat (NAN);
			sendValueAll (windDir);

			pressure->setValueFloat (NAN);
			sendValueAll (pressure);

			rain->setValueFloat (NAN);
			sendValueAll (rain);

			dewpoint->setValueFloat (NAN);
			sendValueAll (dewpoint);

			valueError (lastPool);
		}
		else
		{
			valueGood (lastPool);
		}

		ret = false;
	}
	else
	{
		valueGood (lastPool);
	}
	if (humidity->getValueFloat () > humidity_limit->getValueFloat ())
	{
	  	if (ret)
			setWeatherTimeout (wait_humidity->getValueInteger (), "humidity is above limit");
	  	valueError (humidity);
		ret = false;
	}
	else
	{
		valueGood (humidity);
	}
	if (windSpeed->getValueFloat () > windSpeed_limit->getValueFloat ())
	{
		if (ret)
			setWeatherTimeout (wait_wind->getValueInteger (), "windspeed is above limit");
		valueError (windSpeed);
		ret = false;
	}
	else
	{
		valueGood (windSpeed);
	}
	if (windGustSpeed->getValueFloat () > windGustSpeed_limit->getValueFloat ())
	{
		if (ret)
			setWeatherTimeout (wait_wind->getValueInteger (), "wind gust speed is above limit");
		valueError (windGustSpeed);
		ret = false;
	}
	else
	{
		valueGood (windGustSpeed);
	}
	if (ret == false)
		return ret;  
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	FlwoWeather device (argc, argv);
	return device.run ();
}
