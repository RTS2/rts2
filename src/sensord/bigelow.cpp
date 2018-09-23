/* 
 * LBL Bigelow weather data.
 * Copyright (C) 2018 Petr Kubanek <petr@kubanek.net>
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
#include "json.hpp"

#include <curl/curl.h>

#define BIGELOW_URL "https://www.lpl.arizona.edu/~css/bigelow/boltwoodlast.json"

namespace rts2sensord
{

/**
 * Parses LPL's JSON Bigelow enviromental data and make them accessible inside RTS2.
 * Note that JSON URL is so far hardcoded. Can be used as an example how to integrate 
 * other JSON services.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Bigelow:public SensorWeather
{
	public:
		Bigelow (int argc, char **argv);
		virtual ~Bigelow ();

	protected:
		virtual int initHardware () override;
		virtual bool isGoodWeather () override;

		virtual int info() override;

	private:
		rts2core::ValueFloat *skyTemp;
		rts2core::ValueFloat *ambientTemp;
		rts2core::ValueFloat *sensorTemp;
		rts2core::ValueFloat *windSpeed;
		rts2core::ValueInteger *humidity;
		rts2core::ValueFloat *dewPoint;
		rts2core::ValueInteger *heater;
		rts2core::ValueBool *rain;
		rts2core::ValueBool *wet;
		rts2core::ValueInteger *sinceSec;
		rts2core::ValueInteger *cloud;
		rts2core::ValueInteger *wind;
		rts2core::ValueInteger *iRain;
		rts2core::ValueInteger *daylight;
		rts2core::ValueInteger *close;
		CURL *curl;
		char curl_error[CURL_ERROR_SIZE];
		std::string curl_result;
};

}

using namespace rts2sensord;

Bigelow::Bigelow (int argc, char **argv):SensorWeather(argc, argv)
{
	curl = NULL;

	createValue (skyTemp, "SKY", "[F] sky temperature", true);
	createValue (ambientTemp, "AMB_TEMP", "[F] ambient temperature", false);
	createValue (sensorTemp, "SENSOR_TEMP", "[F] sensor temperature", false);
	createValue (windSpeed, "WINDSPEED", "[m/s] windspeed", false);
	createValue (humidity, "HUMIDITY", "[%] humidity", false);
	createValue (dewPoint, "DEW_POINT", "[F] dew point", false);
	createValue (heater, "HEATER", "heater output", false);
	createValue (rain, "RAIN", "raining", false);
	createValue (wet, "WET", "is wet weather", false);
	createValue (sinceSec, "SINCE_SEC", "last mesurement", false);
	createValue (cloud, "CLOUD", "cloud satisfied", false);
	createValue (wind, "WIND", "wind satisfied", false);
	createValue (iRain, "I_RAIN", "rain satisfied", false);
	createValue (daylight, "DAYLIGHT", "daylight time", false);
	createValue (close, "CLOSE", "CSS closed", false);
}

Bigelow::~Bigelow ()
{
	curl_easy_cleanup(curl);
}

static size_t write_callback (void *contents, size_t size, size_t nmemb, void *userp)
{
	(static_cast<std::string*>(userp))->append((char*)contents, size * nmemb);
	return size * nmemb;
}

int Bigelow::initHardware ()
{
	curl = curl_easy_init();
	if (curl == NULL)
		return -1;

	curl_easy_setopt(curl, CURLOPT_URL, BIGELOW_URL);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &curl_result);

	return 0;
}

bool Bigelow::isGoodWeather ()
{
	return SensorWeather::isGoodWeather();
}

int Bigelow::info ()
{
	curl_result = "";
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		logStream(MESSAGE_ERROR) << "while retrieving " << BIGELOW_URL << ": " << curl_error << sendLog;
		return -1;
	}

	try
	{
		auto js = nlohmann::json::parse(curl_result);

		if (js["sTempUnits"] != "F")
		{
			logStream(MESSAGE_ERROR) << "temperature is not reported in F" << sendLog;
			return -1;
		}
		if (js["sSpeedUnits"] != "M")
		{
			logStream(MESSAGE_ERROR) << "speed is not reported in m" << sendLog;
			return -1;
		}

		skyTemp->setValueFloat((float)js["fSkyTemp"]);
		ambientTemp->setValueFloat((float)js["fAmbientTemp"]);
		sensorTemp->setValueFloat((float)js["fSensorTemp"]);
		windSpeed->setValueFloat((float)js["fWindSpeed"]);
		humidity->setValueInteger((int)js["iHumidity"]);
		dewPoint->setValueFloat((float)js["fDewPoint"]);
		heater->setValueInteger((int)js["iHeater"]);
		rain->setValueBool((int)js["bRain"]);
		wet->setValueBool((int)js["bWet"]);
		sinceSec->setValueInteger((int)js["iSinceSec"]);
		cloud->setValueInteger((int)js["iCloud"]);
		wind->setValueInteger((int)js["iWind"]);
		iRain->setValueInteger((int)js["iRain"]);
		daylight->setValueInteger((int)js["iDaylight"]);
		close->setValueInteger((int)js["iClose"]);

	}
	catch (nlohmann::json::exception &ex)
	{
		logStream(MESSAGE_ERROR) << "cannot parse: " << curl_result << sendLog;
		return -1;
	}

	return SensorWeather::info();
}

int main (int argc, char **argv)
{
	Bigelow device (argc, argv);
	return device.run ();
}
