/* 
 * Sensor for Reinhardt weather station
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

#include "sensord.h"

#include "connection/serial.h"

#define REINHARDT_BUFF_SIZE	1024

namespace rts2sensord
{

/**
 * Class for Reinhard device.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Reinhardt:public SensorWeather
{
	public:
		Reinhardt (int argc, char **argv);
		virtual ~Reinhardt ();

		virtual void addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set);
		virtual void selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual bool isGoodWeather ();
	private:
		char *device_file;
		rts2core::ConnSerial *reinhardtConn;

		std::map <std::string, rts2core::ValueDouble *> reinhardtValues;

		char dataBuff[REINHARDT_BUFF_SIZE];
		int lastReceivedChar;

		rts2core::ValueDouble *temperature;
};

}

typedef struct
{
	const char name[3];
	const char *rts2_name;
	const char *rts2_comment;
} reinhardt_value_t;

reinhardt_value_t r_values[] = {
	{"TE", "TEMP_EXT", "[C] external temperature"},
        {"SO", "GLOBAL_RADIATION", "Global radiation"},
        {"DR", "PRESSURE", "[hPa] pressure"},
        {"ZA", "ADD_1", "Addition1"},
        {"SX", "GLOBAL_RADIATION_EXT", "Global radiation external"},
        {"ZB", "ADD_2", "Addition2"},
        {"WR", "WIND_DIR", "[deg] wind direction"},
        {"ZC", "ADD_3", "Addition3"},
        {"FE", "HUMIDITY", "[%] humidity"},
        {"RE", "RAIN", "rain"},
        {"RD", "RAIN_STORAGE", "rain per storage interval"},
        {"WG", "WIND_SPEED", "[m/sec] wind speed"},
        {"WS", "WIND_PEEK", "[m/sec] wind peek speed"},
        {"WD", "WIND_AVG", "[m/sec] average wind speed"},
        {"GE", "LIGHTNING", "lightning"},
        {"WC", "WINDCHILL", "[C] windchill"},
        {"WV", "WIND_PREVAILING", "[deg] prevailing wind direction"},
        {"GH", "LOCAL_ALTITUDE", "GPS local altitude"},
        {"GX", "GPS_X", "GPS X-coordinate"},
        {"GY", "GPS_Y", "GPS Y-coordinate"},
        {"GV", "GPS_S", "GPS speed"},
        {"TK", "INTERNAL_TEMP", "[mV] internal temperature"},
        {"TR", "REF_VOLT", "[V] internal reference voltage"},
        {"VI", "OPER_VOLT", "[V] operating voltage"},
        {"UH", "HEAT_VOLT", "[V] voltage heating control"},
        {"BF", "LEAF_MOISTURE", "leaf moisture"},
        {"PW", "PF_WATCH", "PF watchdog or PortA"},
        {"PV", "PV_VOLT", "PF voltage reg. or GPS satellites"},
        {"RT", "RAIN_DET", "rain detector"},
        {"WU", "CLOUDS_BASE", "[m] clouds base"},
        {"WK", "CLOUD_DET", "cloud detector"},
        {"WT", "TEMP_SENS", "[C] temperature sensor of cloud detector"},
        {"TD", "INT_TEMP", "[C] internal temperature of pressure sensor"},
        {"UV", "UV_RAD", "[mW/m2] radiation"},
        {"LX", "LIGHT_INTENS", "[lux] light intensity"},
        {"FA", "FAN_SPEED", "speed of fan"},
        {"BA", "TEMP_SOIL_30", "[C] temperature soilsensor 140 at + 30cm"},
        {"BB", "TEMP_SOIL_5", "[C] temperature soilsensor 140 at + 5cm"},
        {"BC", "TEMP_SOIL_M5", "[C] temperature soilsensor 140 at - 5cm"},
        {"BD", "TEMP_SOIL_M50", "[C] temperature soilsensor 140 at - 50cm"},
        {"BE", "TEMP_SOIL_M100", "[C] temperature soilsensor 140 at - 100cm"},
	{"  ", NULL, NULL}
};

using namespace rts2sensord;

Reinhardt::Reinhardt (int argc, char **argv):SensorWeather (argc, argv)
{
	device_file = NULL;
	reinhardtConn = NULL;

	lastReceivedChar = 0;

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
}

Reinhardt::~Reinhardt ()
{
	delete reinhardtConn;
}

void Reinhardt::addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	reinhardtConn->add (&read_set, &write_set, &exp_set);
	SensorWeather::addSelectSocks (read_set, write_set, exp_set);
}

void Reinhardt::selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	if (reinhardtConn->receivedData (&read_set))
	{
		if (lastReceivedChar == REINHARDT_BUFF_SIZE) // buffer full..
		{
			lastReceivedChar = 0;
		}

		int ret = reinhardtConn->readPort (dataBuff + lastReceivedChar, REINHARDT_BUFF_SIZE - lastReceivedChar, '\n');
		if (ret > 0 && dataBuff[lastReceivedChar + ret - 1] == '\n')
		{
			dataBuff[lastReceivedChar + ret] = '\0';
			struct tm lastInfoTime;
			ret = sscanf (dataBuff, "%d:%d:%d, %d.%d.%d", &lastInfoTime.tm_hour, &lastInfoTime.tm_min, &lastInfoTime.tm_sec, &lastInfoTime.tm_mday, &lastInfoTime.tm_mon, &lastInfoTime.tm_year);
			if (ret != 6)
			{
				logStream (MESSAGE_ERROR) << "cannot parse date from " << dataBuff << sendLog;
			}
			else
			{
				setInfoTime (mktime (&lastInfoTime));

				// now parse it..
				int commaCount = 0;
				for (char *p = dataBuff; *p != '\0'; p++)
				{
					// first two entries are time and date, skip them
					if (commaCount < 2)
					{
						if (*p == ',')
							commaCount++;
					}
					else
					{
						// skip blanks
						while (*p == ' ')
							p++;
						// reached end..
						if (*p == '\0')
							break;
						// try to find two letter identifier
						char vname[3];
						memcpy (vname, p, 2);
						vname[2] = '\0';
						std::map <std::string, rts2core::ValueDouble *>::iterator it = reinhardtValues.find (vname);
						rts2core::ValueDouble *dval = NULL;

						// try to parse value
						char *vend = p;
						while (*vend != ',' && *vend != '\0')
							vend++;
						// mark end ,
						*vend = '\0';

						// if we cannot find value in map, create it
						if (it == reinhardtValues.end ())
						{
							// try to find vname in r_values
							reinhardt_value_t *rv;
							for (rv = r_values; rv->name != NULL; rv++)
							{
								if (strcmp (rv->name, vname) == 0)
									break;
							}
							if (rv->name != NULL)
							{
								dval = new rts2core::ValueDouble (rv->rts2_name, rv->rts2_comment, false);
								addValue (dval);
								updateMetaInformations (dval);
							}
						}
						else
						{
							dval = it->second;
						}
						if (dval == NULL)
						{
							logStream (MESSAGE_ERROR) << "cannot find value with name " << dval << sendLog;
						}
						else
						{
							dval->setValueDouble (atof (p + 2));
							p = vend; // ++ in for will shift us
						}
					}
				}
			}

			// make sure we start with empty received buffer
			lastReceivedChar = 0;
		}
	}

	SensorWeather::selectSuccess (read_set, write_set, exp_set);
}

int Reinhardt::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int Reinhardt::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	reinhardtConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 30);
	int ret = reinhardtConn->init ();
	if (ret)
		return ret;
	reinhardtConn->setDebug (getDebug ());
	reinhardtConn->writePort ('?');
	return ret;
}

bool Reinhardt::isGoodWeather ()
{
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	Reinhardt device (argc, argv);
	return device.run ();
}
