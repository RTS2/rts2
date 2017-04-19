/* 
 * Driver for FLWO MEarth weather station.
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

class FlwoMe;

class MEarthWeather:public rts2core::ConnUDP
{
	public:
		MEarthWeather (int port, FlwoMe *master);
	protected:
		virtual int process (size_t len, struct sockaddr_in &from);
	private:
		time_t paramNextTimeME ();

		double paramNextDouble ();

		void paramNextFloatME (rts2core::ValueFloat *val);
		void paramNextSkyME (rts2core::ValueDoubleStat *val, rts2core::ValueBool *rainv, int nv);
};

class FlwoMe:public SensorWeather
{
	public:
		FlwoMe (int argc, char **argv);
		virtual ~FlwoMe ();
	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

		virtual bool isGoodWeather ();

	private:
		int me_port;
		MEarthWeather *mearth;

		rts2core::ValueFloat *wait_nodata;
		rts2core::ValueFloat *wait_skytemp;
		rts2core::ValueFloat *wait_me_rain;

		rts2core::ValueFloat *ignore_nodata;

		// ME values
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
		rts2core::ValueBool *me_rain;

		friend class MEarthWeather;
};

MEarthWeather::MEarthWeather (int _port, FlwoMe *_master):ConnUDP (_port, _master, NULL, 500)
{
};

time_t MEarthWeather::paramNextTimeME ()
{
	char *str_num;
	char *endptr;
	if (paramNextString (&str_num, ","))
	{
		throw rts2core::Error ("cannot get value");
	}
	double mjd = strtod (str_num, &endptr);
	if (*endptr == '\0')
	{
		time_t t;
		ln_get_timet_from_julian (mjd + JD_TO_MJD_OFFSET, &t);
		return t;
	}
	if (strcmp (str_num, "---") == 0)
	{
		return -1;
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

void MEarthWeather::paramNextSkyME (rts2core::ValueDoubleStat *val, rts2core::ValueBool *rainv, int nv)
{
	double v = paramNextDouble ();
	if (std::isnan (v))
	{
		logStream (MESSAGE_WARNING) << "nan weather value received" << sendLog;
		return;
	}
	if (v < -400)
	{
		logStream (MESSAGE_DEBUG) << "value below -400, wet sensor?" << sendLog;
		rainv->setValueBool (true);
		return;
	}
	rainv->setValueBool (false);
	val->addValue (v, nv);
	val->calculate ();
}

int MEarthWeather::process (size_t len, struct sockaddr_in &from)
{
	try
	{
		((FlwoMe *) master)->setInfoTime (paramNextTimeME ());
		paramNextFloatME (((FlwoMe *) master)->me_winddir);
		paramNextFloatME (((FlwoMe *) master)->me_windspeed);
		paramNextFloatME (((FlwoMe *) master)->me_temp);
		paramNextFloatME (((FlwoMe *) master)->me_dewpoint);
		paramNextFloatME (((FlwoMe *) master)->me_humidity);
		paramNextFloatME (((FlwoMe *) master)->me_pressure);
		paramNextFloatME (((FlwoMe *) master)->me_rain_accumulation);
		paramNextFloatME (((FlwoMe *) master)->me_rain_duration);
		paramNextFloatME (((FlwoMe *) master)->me_rain_intensity);
		paramNextFloatME (((FlwoMe *) master)->me_hail_accumulation);
		paramNextFloatME (((FlwoMe *) master)->me_hail_duration);
		paramNextFloatME (((FlwoMe *) master)->me_hail_intensity);
		paramNextSkyME (((FlwoMe *) master)->me_sky_temp, ((FlwoMe *) master)->me_rain, ((FlwoMe *) master)->me_sky_avg->getValueInteger ());
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_DEBUG) << "cannot parse MEarth UDP packet, rest contet is " << buf << ", erorr is " << er << sendLog;
		return -1;
	}
	return 0;
}

FlwoMe::FlwoMe (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (wait_nodata, "wait_nodata", "[s] set bad weather when data are not refreshed after this seconds", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (wait_skytemp, "wait_skytemp", "[s] wait for this number of seconds if skytemp is outside limit", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (wait_me_rain, "wait_me_rain", "[s] wait time when ME detects rain", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	createValue (ignore_nodata, "ignore_nodata", "[s] no data packet will be ignored for the given period", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	wait_nodata->setValueFloat (300);
	wait_skytemp->setValueFloat (900);
	wait_me_rain->setValueFloat (900);

	ignore_nodata->setValueInteger (300);

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

	createValue (me_rain, "me_rain", "true if sky temp sensor detect rain", false);

	me_port = 0;
	mearth = NULL;

	addOption ('m', NULL, 1, "MEarth port number");

	setIdleInfoInterval (15);
}

FlwoMe::~FlwoMe ()
{
}

int FlwoMe::processOption (int opt)
{
	switch (opt)
	{
		case 'm':
			me_port = atoi (optarg);
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int FlwoMe::initHardware ()
{
	if (me_port > 0)
	{
		mearth = new MEarthWeather (me_port, this);
		int ret = mearth->init ();
		if (ret)
			return ret;
		addConnection (mearth);
	}
	else
	{
		logStream(MESSAGE_ERROR) << "cannot init MEarth connection, as port is negative value " << me_port << sendLog;
		return -1;
	}
	return 0;
}

int FlwoMe::info ()
{
	return 0;
}

bool FlwoMe::isGoodWeather ()
{
	bool ret = true;
	if (getLastInfoTime () > ignore_nodata->getValueFloat ())
  	{
		setWeatherTimeout (wait_nodata->getValueInteger (), "weather data not received");

		me_winddir->setValueFloat (NAN);
		sendValueAll (me_winddir);

		me_windspeed->setValueFloat (NAN);
		sendValueAll (me_windspeed);

		me_temp->setValueFloat (NAN);
		sendValueAll (me_temp);

		me_dewpoint->setValueFloat (NAN);
		sendValueAll (me_dewpoint);

		me_pressure->setValueFloat (NAN);
		sendValueAll (me_pressure);

		me_rain_accumulation->setValueFloat (NAN);
		sendValueAll (me_rain_accumulation);

		me_rain_duration->setValueFloat (NAN);
		sendValueAll (me_rain_duration);

		me_rain_intensity->setValueFloat (NAN);
		sendValueAll (me_rain_intensity);

		me_hail_accumulation->setValueFloat (NAN);
		sendValueAll (me_hail_accumulation);

		me_hail_duration->setValueFloat (NAN);
		sendValueAll (me_hail_duration);

		me_hail_intensity->setValueFloat (NAN);
		sendValueAll (me_hail_intensity);

		me_sky_temp->clearStat ();
		valueError (me_sky_temp);
		sendValueAll (me_sky_temp);

		me_rain->setValueInteger (0);
		valueError (me_rain);
		sendValueAll (me_rain);

		ret = false;
	}
	if (me_sky_temp->getValueFloat () > me_sky_limit->getValueFloat ())
	{
		if (ret)
		{
			std::ostringstream _os;
			_os << "SKYTEMP " << me_sky_temp->getValueFloat () << "C, above limit of " << me_sky_limit->getValueFloat () << "C";
			setWeatherTimeout (wait_skytemp->getValueInteger (), _os.str ().c_str ());
		}
		valueError (me_sky_temp);
		ret = false;
	}
	else
	{
		valueGood (me_sky_temp);
	}
	if (me_rain->getValueBool ())
	{
		if (ret)
			setWeatherTimeout (wait_me_rain->getValueInteger (), "ME RAIN");
		valueError (me_rain);
		ret = false;
	}
	else
	{
		valueGood (me_rain);
	}
	if (ret == false)
		return ret;  
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	FlwoMe device (argc, argv);
	return device.run ();
}
