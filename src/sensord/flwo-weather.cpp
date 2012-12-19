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
#include "connection/udp.h"

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
	private:
		void paramNextTimeME (rts2core::ValueTime *val);

		double paramNextDouble ();

		void paramNextFloatME (rts2core::ValueFloat *val);
		void paramNextDoubleStatME (rts2core::ValueDoubleStat *val, int nv);
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

		// wait values
		rts2core::ValueFloat *wait_nodata;
		rts2core::ValueFloat *wait_humidity;
		rts2core::ValueFloat *wait_wind;
		rts2core::ValueFloat *wait_skytemp;

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
		rts2core::ValueFloat *dewpoint;
		rts2core::ValueBool *hatRain;
		rts2core::ValueTime *lastPool;

		// ME values
		rts2core::ValueTime *me_mjd;
		rts2core::ValueFloat *me_winddir;
		rts2core::ValueFloat *me_windspeed;
		rts2core::ValueFloat *me_temp;
		rts2core::ValueFloat *me_dewpoint;
		rts2core::ValueFloat *me_humidity;
		rts2core::ValueFloat *me_pressure;
		rts2core::ValueFloat *me_rain_accumulation;
		rts2core::ValueFloat *me_rain_duration;
		rts2core::ValueFloat *me_rain_intensity;
		rts2core::ValueFloat *me_hail_accumulation;
		rts2core::ValueFloat *me_hail_duration;
		rts2core::ValueFloat *me_hail_intensity;
		rts2core::ValueDoubleStat *me_sky_temp;
		rts2core::ValueInteger *me_sky_avg;
		rts2core::ValueFloat *me_sky_limit;

		friend class MEarthWeather;
};

MEarthWeather::MEarthWeather (int _port, FlwoWeather *_master):ConnUDP (_port, _master, 500)
{
};

void MEarthWeather::paramNextTimeME (rts2core::ValueTime *val)
{
	char *str_num;
	char *endptr;
	if (paramNextString (&str_num, ","))
	{
		throw rts2core::Error ("cannot get value");
		return;
	}
	double mjd = strtod (str_num, &endptr);
	if (*endptr == '\0')
	{
		time_t t;
		ln_get_timet_from_julian (mjd + JD_TO_MJD_OFFSET, &t);
		val->setValueDouble (t);
		return;
	}
	if (strcmp (str_num, "---") == 0)
	{
		val->setValueDouble (NAN);
		return;
	}
	throw rts2core::Error ("cannot parse " + std::string (str_num));
}

double MEarthWeather::paramNextDouble ()
{
	char *str_num;
	char *endptr;
	if (paramNextString (&str_num, ","))
	{
		throw rts2core::Error ("cannot get value");
	}
	double val = strtod (str_num, &endptr);
	if (*endptr == '\0')
		return val;
	if (strcmp (str_num, "---") == 0)
	{
		return NAN;
	}
	throw rts2core::Error ("cannot parse " + std::string (str_num));
}

void MEarthWeather::paramNextFloatME (rts2core::ValueFloat *val)
{
	val->setValueFloat (paramNextDouble ());
}

void MEarthWeather::paramNextDoubleStatME (rts2core::ValueDoubleStat *val, int nv)
{
	double v = paramNextDouble ();
	if (isnan (v))
		return;
	val->addValue (v, nv);
	val->calculate ();
}

int MEarthWeather::process (size_t len, struct sockaddr_in &from)
{
	try
	{
		paramNextTimeME (((FlwoWeather *) master)->me_mjd);
		paramNextFloatME (((FlwoWeather *) master)->me_winddir);
		paramNextFloatME (((FlwoWeather *) master)->me_windspeed);
		paramNextFloatME (((FlwoWeather *) master)->me_temp);
		paramNextFloatME (((FlwoWeather *) master)->me_dewpoint);
		paramNextFloatME (((FlwoWeather *) master)->me_humidity);
		paramNextFloatME (((FlwoWeather *) master)->me_pressure);
		paramNextFloatME (((FlwoWeather *) master)->me_rain_accumulation);
		paramNextFloatME (((FlwoWeather *) master)->me_rain_duration);
		paramNextFloatME (((FlwoWeather *) master)->me_rain_intensity);
		paramNextFloatME (((FlwoWeather *) master)->me_hail_accumulation);
		paramNextFloatME (((FlwoWeather *) master)->me_hail_duration);
		paramNextFloatME (((FlwoWeather *) master)->me_hail_intensity);
		paramNextDoubleStatME (((FlwoWeather *) master)->me_sky_temp, ((FlwoWeather *) master)->me_sky_avg->getValueInteger ());
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_DEBUG) << "cannot parse MEarth UDP packet, rest contet is " << buf << ", erorr is " << er << sendLog;
		((FlwoWeather *) master)->me_mjd->setValueDouble (NAN);
		return -1;
	}
	return 0;
}

FlwoWeather::FlwoWeather (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (wait_nodata, "wait_nodata", "[s] set bad weather when data are not refreshed after this seconds", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (wait_humidity, "wait_humidity", "[s] set bad weather when humidity is over limit for this seconds", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (wait_wind, "wait_wind", "[s] set bad weather when wind is over limit for this number of seconds", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (wait_skytemp, "wait_skytemp", "[s] wait for this number of seconds if skytemp is outside limit", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	wait_nodata->setValueFloat (300);
	wait_humidity->setValueFloat (300);
	wait_wind->setValueFloat (300);
	wait_skytemp->setValueFloat (900);

	createValue (outsideTemp, "outside_temp", "[C] outside temperature", false);
	createValue (windSpeed, "wind_speed", "[mph] windspeed", false);
	createValue (windSpeedAvg, "wind_speed_avg", "number of measurements to average", false, RTS2_VALUE_WRITABLE);
	windSpeedAvg->setValueInteger (20);
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
	createValue (rain, "rain", "[inch] total accumulated rain", false);
	createValue (dewpoint, "dewpoint", "[C] dewpoint", false);
	createValue (hatRain, "HAT_rain", "hat rain status", false);
	createValue (lastPool, "last_pool", "last file pool", false);

	createValue (me_mjd, "me_mjd", "MEarth MJD", false);
	createValue (me_winddir, "me_winddir", "MEarth wind direction", false);
	createValue (me_windspeed, "me_windspeed", "MEarth wind speed", false);
	createValue (me_temp, "me_temp", "MEarth temperature", false);
	createValue (me_dewpoint, "me_dewpoint", "MEarth dewpoint", false);
	createValue (me_humidity, "me_humidity", "MEarth humidity", false);
	createValue (me_pressure, "me_pressure", "MEarth atmospheric pressure", false);
	createValue (me_rain_accumulation, "me_rain_accumulation", "MEarth rain accumulation", false);
	createValue (me_rain_duration, "me_rain_duration", "MEarth rain duration", false);
	createValue (me_rain_intensity, "me_rain_intensity", "MEarth rain intensity", false);
	createValue (me_hail_accumulation, "me_hail_accumulation", "MEarth hail accumulation", false);
	createValue (me_hail_duration, "me_hail_duration", "MEarth hail duration", false);
	createValue (me_hail_intensity, "me_hail_intensity", "MEarth hail intensity", false);
	createValue (me_sky_temp, "me_sky_temp", "MEarth sky temperature", false);
	createValue (me_sky_avg, "me_sky_avg", "number of MEarth sky temperature values to average", false);
	me_sky_avg->setValueInteger (10);
	createValue (me_sky_limit, "me_sky_limit", "sky limit (if sky_temp < sky_limit, there aren't clouds)", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	me_sky_limit->setValueFloat (-20);

	weatherFile = NULL;

	me_port = 0;
	mearth = NULL;

	addOption ('f', NULL, 1, "file with weather informations");
	addOption ('m', NULL, 1, "MEarth port number");

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
				char *endptr;
				double v = strtod (ch, &endptr);
				if (endptr != ch && *endptr == '\0')
				{
					windSpeed->addValue (v, windSpeedAvg->getValueInteger ());
					windSpeed->calculate ();
					processed |= 1 << 1;
				}
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
	{
		lastPool->setValueInteger (et);
		if (!isnan (me_mjd->getValueDouble ()) && et < me_mjd->getValueDouble ())
			setInfoTime (et);
		else
			setInfoTime (me_mjd->getValueDouble ());
	}
	else
	{
		setInfoTime (NAN);
		return -1;
	}

	return 0;
}

bool FlwoWeather::isGoodWeather ()
{
  	bool ret = true;
	if (getLastInfoTime () > 60)
  	{
	  	setWeatherTimeout (wait_nodata->getValueInteger (), "weather data not recived");
		ret = false;
	}
	if (humidity->getValueFloat () > humidity_limit->getValueFloat ())
	{
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
		setWeatherTimeout (wait_wind->getValueInteger (), "wind gust speed is above limit");
		valueError (windGustSpeed);
		ret = false;
	}
	else
	{
		valueGood (windGustSpeed);
	}
	if (me_sky_temp->getValueFloat () > me_sky_limit->getValueFloat ())
	{
		std::ostringstream _os;
		_os << "SKYTEMP " << me_sky_temp->getValueFloat () << "C, above limit of " << me_sky_limit->getValueFloat () << "C";
		setWeatherTimeout (wait_skytemp->getValueInteger (), _os.str ().c_str ());
		valueError (me_sky_temp);
		ret = false;
	}
	else
	{
		valueGood (me_sky_temp);
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
