/* 
 * Driver for FLWO weather station.
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
#include "../utils/connudp.h"

#include <fstream>
#include <sstream>

using namespace rts2sensord;

class FlwoWeather;

class MEarthWeather:public rts2core::ConnUDP
{
	public:
		MEarthWeather (int port, FlwoWeather *master);
	protected:
		virtual int process (size_t len, struct sockaddr_in &from);
};

class FlwoWeather:public SensorWeather
{
	public:
		FlwoWeather (int argc, char **argv);
		virtual ~FlwoWeather ();
	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual bool isGoodWeather ();

	private:
		const char *weatherFile;
		int me_port;
		MEarthWeather *mearth;

		Rts2ValueFloat *outsideTemp;
		Rts2ValueFloat *windSpeed;
		Rts2ValueFloat *windSpeed_limit;
		Rts2ValueFloat *windGustSpeed;
		Rts2ValueFloat *windGustSpeed_limit;
		Rts2ValueFloat *windDir;
		Rts2ValueFloat *pressure;
		Rts2ValueFloat *humidity;
		Rts2ValueFloat *humidity_limit;
		Rts2ValueFloat *rain;
		Rts2ValueFloat *dewpoint;
		Rts2ValueBool *hatRain;

		// ME values
		Rts2ValueDouble *me_mjd;
		Rts2ValueFloat *me_winddir;
		Rts2ValueFloat *me_windspeed;
		Rts2ValueFloat *me_temp;
		Rts2ValueFloat *me_humidity;
		Rts2ValueFloat *me_pressure;
		Rts2ValueFloat *me_rain_accumulation;
		Rts2ValueFloat *me_rain_duration;
		Rts2ValueFloat *me_rain_intensity;
		Rts2ValueFloat *me_hail_accumulation;
		Rts2ValueFloat *me_hail_duration;
		Rts2ValueFloat *me_hail_intensity;
		Rts2ValueFloat *me_sky_temp;

		friend class MEarthWeather;
};

MEarthWeather::MEarthWeather (int _port, FlwoWeather *_master):ConnUDP (_port, _master, 500)
{
};

int MEarthWeather::process (size_t len, struct sockaddr_in &from)
{
	double _mjd;
	float _winddir, _windspeed, _temp, _humidity, _pressure, _rain_accumulation, _rain_duration, _rain_intensity, _hail_accumulation, _hail_duration, _hail_intensity, _sky_temp;
	std::istringstream is (buf);
	is >> _mjd >> _winddir >> _windspeed >> _temp >> _humidity >> _pressure >> _rain_accumulation >> _rain_duration >> _rain_intensity >> _hail_accumulation >> _hail_duration >> _hail_intensity >> _sky_temp;
	if (is.fail ())
	{
		logStream (MESSAGE_DEBUG) << "cannot process MEarth UDP socket: " << buf << sendLog;
		return -1;
	}
	((FlwoWeather *) master)->me_mjd->setValueDouble (_mjd);
	((FlwoWeather *) master)->me_winddir->setValueDouble (_winddir);
	((FlwoWeather *) master)->me_windspeed->setValueDouble (_windspeed);
	((FlwoWeather *) master)->me_temp->setValueDouble (_temp);
	((FlwoWeather *) master)->me_humidity->setValueDouble (_humidity);
	((FlwoWeather *) master)->me_pressure->setValueDouble (_pressure);
	((FlwoWeather *) master)->me_rain_accumulation->setValueDouble (_rain_accumulation);
	((FlwoWeather *) master)->me_rain_duration->setValueDouble (_rain_duration);
	((FlwoWeather *) master)->me_rain_intensity->setValueDouble (_rain_intensity);
	((FlwoWeather *) master)->me_hail_accumulation->setValueDouble (_hail_accumulation);
	((FlwoWeather *) master)->me_hail_duration->setValueDouble (_hail_duration);
	((FlwoWeather *) master)->me_hail_intensity->setValueDouble (_hail_intensity);
	((FlwoWeather *) master)->me_sky_temp->setValueDouble (_sky_temp);
	return 0;
}

FlwoWeather::FlwoWeather (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (outsideTemp, "outside_temp", "[C] outside temperature", false);
	createValue (windSpeed, "wind_speed", "[mph] windspeed", false);
	createValue (windSpeed_limit, "wind_speed_limit", "[mph] windspeed limit", false, RTS2_VALUE_WRITABLE);
	windSpeed_limit->setValueFloat (40);

	createValue (windGustSpeed, "wind_gust", "[mph] wind gust speed", false);
	createValue (windGustSpeed_limit, "wind_gust_limit", "[mph] wind gust speed limit", false, RTS2_VALUE_WRITABLE);
	windGustSpeed_limit->setValueFloat (45);

	createValue (windDir, "wind_direction", "[deg] wind direction", false);
	createValue (pressure, "pressure", "[mB] atmospheric pressure", false);
	createValue (humidity, "humidity", "[%] outside humidity", false);
	createValue (humidity_limit, "humidity_limit", "[%] humidity limit for bad weather", false, RTS2_VALUE_WRITABLE);
	humidity_limit->setValueFloat (90);
	createValue (rain, "rain", "[inch] total accumulated rain", false);
	createValue (dewpoint, "dewpoint", "[C] dewpoint", false);
	createValue (hatRain, "HAT_rain", "hat rain status", false);

	createValue (me_mjd, "me_mjd", "MEarth MJD", false);
	createValue (me_winddir, "me_winddir", "MEarth wind direction", false);
	createValue (me_windspeed, "me_windspeed", "MEarth wind speed", false);
	createValue (me_temp, "me_temp", "MEarth temperature", false);
	createValue (me_humidity, "me_humidity", "MEarth humidity", false);
	createValue (me_pressure, "me_pressure", "MEarth atmospheric pressure", false);
	createValue (me_rain_accumulation, "me_rain_accumulation", "MEarth rain accumulation", false);
	createValue (me_rain_duration, "me_rain_duration", "MEarth rain duration", false);
	createValue (me_rain_intensity, "me_rain_intensity", "MEarth rain intensity", false);
	createValue (me_hail_accumulation, "me_hail_accumulation", "MEarth hail accumulation", false);
	createValue (me_hail_duration, "me_hail_duration", "MEarth hail duration", false);
	createValue (me_hail_intensity, "me_hail_intensity", "MEarth hail intensity", false);
	createValue (me_sky_temp, "me_sky_temp", "MEarth sky temperature", false);

	weatherFile = NULL;

	me_port = 0;
	mearth = NULL;

	addOption ('f', NULL, 1, "file with weather informations");
	addOption ('m', NULL, 1, "MEarth port number");
}

FlwoWeather::~FlwoWeather ()
{
	delete mearth;
}

int FlwoWeather::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			weatherFile = optarg;
			break;
		case 'm':
			me_port = atoi (optarg);
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int FlwoWeather::init ()
{
	int ret = SensorWeather::init ();
	if (ret)
		return ret;
	if (me_port > 0)
	{
		mearth = new MEarthWeather (me_port, this);
		ret = mearth->init ();
		if (ret)
			return ret;
		addConnection (mearth);
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
	if (humidity->getValueFloat () > humidity_limit->getValueFloat ())
	{
	  	setWeatherTimeout (600, "humidity is above limit");
		return false;
	}
	if (windSpeed->getValueFloat () > windSpeed_limit->getValueFloat ())
	{
		setWeatherTimeout (600, "windspeed is above limit");
	}
	if (windGustSpeed->getValueFloat () > windGustSpeed_limit->getValueFloat ())
	{
		setWeatherTimeout (600, "wind gust speed is above limit");
	}
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	FlwoWeather device (argc, argv);
	return device.run ();
}
