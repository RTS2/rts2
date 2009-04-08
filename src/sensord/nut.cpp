/* 
 * NUT UPS deamon sensor.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/conntcp.h"

#include <map>

namespace rts2sensor
{

	/**
	 * Class for communication with NUT.
	 *
	 * @author Petr Kubanek <petr@kubanek.net>
	 */
	class ConnNUT: public rts2core::ConnTCP
	{
		private:
			std::map <std::string, std::string> values;

		public:
			/**
			 * Create new connection to APC UPS daemon.
			 *
			 * @param _master   Reference to master holding this connection.
			 *
			 * @param _hostname APC UPSD IP address or hostname.
			 * @param _port     Portnumber of APC UPSD daemon (default to 3493).
			 */
			ConnNUT (Rts2Block *_master, const char *_hostname, int _port);

			/**
			 * Call command, get reply.
			 *
			 * @param cmd       Command.
			 */
			int command (const char *cmd);

			const char *getString (const char *val);
			float getPercents (const char *val);
			float getTemp (const char *val);
			int getTime (const char *val);
	};

	/**
	 * Sensor for accessing APC UPS informations.
	 *
	 * @author Petr Kubanek <petr@kubanek.net>
	 */
	class NUT:public SensorWeather
	{
		private:
			HostString *host;
			ConnNUT *connNUT;

			Rts2ValueString *model;

			Rts2ValueFloat *loadpct;
			Rts2ValueFloat *bcharge;
			Rts2ValueInteger *timeleft;
			Rts2ValueFloat *itemp;
			Rts2ValueInteger *tonbatt;
			Rts2ValueString *status;

			Rts2ValueInteger *battimeout;

			Rts2ValueFloat *minbcharge;
			Rts2ValueInteger *mintimeleft;

		protected:
			virtual int processOption (int opt);
			virtual int info ();

			virtual int init ();

		public:
			NUT (int argc, char **argv);
			virtual ~NUT (void);
	};

}


using namespace rts2sensor;


ConnNUT::ConnNUT (Rts2Block *_master, const char *_hostname, int _port)
:rts2core::ConnTCP (_master, _hostname, _port)
{
}


int
ConnNUT::command (const char *cmd)
{
	std::istringstream *_is = NULL;
	try
	{
		sendData (cmd);
		sendData ("\n");
		
		receiveData (&_is, 2, '\n');

		std::string str;

		*_is >> str;
		std::cout << str << std::endl;

		delete _is;
		return 0;
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		delete _is;
		return -1;
	}
}


const char*
ConnNUT::getString (const char *val)
{
	std::map <std::string, std::string>::iterator iter = values.find (val);
	if (values.find (val) == values.end ())
		return "";
	return (*iter).second.c_str ();
}


float
ConnNUT::getPercents (const char *val)
{
	std::map <std::string, std::string>::iterator iter = values.find (val);
	if (values.find (val) == values.end ())
		return nan("f");
	return atof ((*iter).second.c_str());
}


float
ConnNUT::getTemp (const char *val)
{
	const char *v = getString (val);
	if (strchr (v, 'C') == NULL)
		return nan("f");
	return atof (v);
}


int
ConnNUT::getTime (const char *val)
{
	const char *v = getString (val);
	if (strcasestr (v, "hours") != NULL)
	  	return atof (v) * 3600;
	if (strcasestr (v, "minutes") != NULL)
		return atof (v) * 60;
	if (strcasestr (v, "seconds") != NULL)
	  	return atof (v);
	return 0;
}




int
NUT::processOption (int opt)
{
	switch (opt)
	{
		case 'n':
			host = new HostString (optarg, "3493");
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}


int
NUT::init ()
{
  	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;
	
	connNUT = new ConnNUT (this, host->getHostname (), host->getPort ());
	ret = connNUT->init ();
	if (ret)
		return ret;
	ret = info ();
	if (ret)
		return ret;
	setIdleInfoInterval (10);
	return 0;
}


int
NUT::info ()
{
	int ret;
	ret = connNUT->command ("GET VAR MGEUPS ups.load");
	if (ret)
	{
		logStream (MESSAGE_WARNING) << "cannot retrieve informations from apcups, putting UPS to bad weather state" << sendLog;
		setWeatherTimeout (120);
		return ret;
	}
	if (tonbatt->getValueInteger () > battimeout->getValueInteger ())
	{
		logStream (MESSAGE_WARNING) <<  "too long on batteries: " << tonbatt->getValueInteger () << sendLog;
		setWeatherTimeout (battimeout->getValueInteger () + 60);
	}

	if (bcharge->getValueFloat () < minbcharge->getValueFloat ())
	{
	 	logStream (MESSAGE_WARNING) << "battery charge too low: " << bcharge->getValueFloat () << " < " << minbcharge->getValueFloat () << sendLog;
		setWeatherTimeout (1200);
	}

	if (timeleft->getValueInteger () < mintimeleft->getValueInteger ())
	{
	 	logStream (MESSAGE_WARNING) << "minimal battery time too low: " << timeleft->getValueInteger () << " < " << mintimeleft->getValueInteger () << sendLog;
		setWeatherTimeout (1200);

	}

	// if there is any UPS error, set big timeout..
	if (strcmp (status->getValue (), "ONLINE") && strcmp (status->getValue (), "ONBATT"))
	{
		logStream (MESSAGE_WARNING) <<  "unknow status " << status->getValue () << sendLog;
		setWeatherTimeout (1200);
	}
	
	return SensorWeather::info ();
}


NUT::NUT (int argc, char **argv):SensorWeather (argc, argv)
{
  	createValue (model, "model", "UPS mode", false);
	createValue (loadpct, "load", "UPS load", false);
	createValue (bcharge, "bcharge", "battery charge", false);
	createValue (timeleft, "timeleft", "time left for on-UPS operations", false);
	createValue (itemp, "temperature", "internal UPS temperature", false);
	createValue (tonbatt, "tonbatt", "time on battery", false);
	createValue (status, "status", "UPS status", false);

	createValue (battimeout, "battery_timeout", "shorter then those onbatt interruptions will be ignored", false);
	battimeout->setValueInteger (60);

	createValue (minbcharge, "min_bcharge", "minimal battery charge for opening", false);
	minbcharge->setValueFloat (50);
	createValue (mintimeleft, "min_tleft", "minimal time left for UPS operation", false);
	mintimeleft->setValueInteger (1200);

	addOption ('n', NULL, 1, "hostname[:port] of NUT");
}


NUT::~NUT (void)
{
	delete connNUT;
}


int
main (int argc, char **argv)
{
	NUT device = NUT (argc, argv);
	return device.run ();
}
