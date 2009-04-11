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

#define OPT_MINBCHARGE   OPT_LOCAL + 221
#define OPT_MINBRUNTIM   OPT_LOCAL + 222
#define OPT_MAXONBAT     OPT_LOCAL + 223

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
			std::string upsName;

			template <typename t> void getVal (const char *var, t &val);
		public:
			/**
			 * Create new connection to NUT UPS daemon.
			 *
			 * @param _master   Reference to master holding this connection.
			 *
			 * @param _hostname NUT UPSD IP address or hostname.
			 * @param _port     Portnumber of NUP UPS daemon (default to 3493).
			 * @param _upsName  Name of UPS.
			 */
			ConnNUT (Rts2Block *_master, const char *_hostname, int _port, const char *_upsName);

			/**
			 * Call GET VAR command to receive variable from UPS.
			 *
			 * @param var    Variable name.
			 * @param value  Variable value.
			 *
			 * @throw ConnError
			 */
			void getValue (const char *var, Rts2ValueFloat *value);

			void getValue (const char *var, Rts2ValueInteger *value);
			void getValue (const char *var, Rts2ValueString *value);
	};

	/**
	 * Sensor for accessing NUT UPS informations.
	 *
	 * @author Petr Kubanek <petr@kubanek.net>
	 */
	class NUT:public SensorWeather
	{
		private:
			HostString *host;
			const char *upsName;
			ConnNUT *connNUT;

			Rts2ValueString *model;

			Rts2ValueFloat *loadpct;
			Rts2ValueFloat *bcharge;
			Rts2ValueInteger *bruntime;

			Rts2ValueTime *onbatterytimeout;

			Rts2ValueString *upsstatus;
			
			Rts2ValueFloat *minbcharge;
			Rts2ValueInteger *mintimeleft;
			Rts2ValueInteger *maxonbattery;

		protected:
			virtual int processOption (int opt);
			virtual int info ();

			virtual int init ();

			virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);

		public:
			NUT (int argc, char **argv);
			virtual ~NUT (void);
	};

}


using namespace rts2sensor;


ConnNUT::ConnNUT (Rts2Block *_master, const char *_hostname, int _port, const char *_upsName)
:rts2core::ConnTCP (_master, _hostname, _port)
{
	upsName = std::string (_upsName);
}


template <typename t> void
ConnNUT::getVal (const char *var, t &val)
{
	std::istringstream *_is = NULL;

	sendData (std::string ("GET VAR ") + upsName
		+ " " + std::string (var) + std::string ("\n"));
	receiveData (&_is, 2, '\n');

	std::string var_str, ups_name, var_name;
	char ap;

	*_is >> var_str;

	if (var_str != "VAR")
	{
		delete _is;
		throw rts2core::ConnError ((std::string ("Error getting ") + var + " " + var_str).c_str ());
	}
	
	*_is >> ups_name >> var_name >> ap >> val >> ap;
	if (!(_is->good () && ap == '"'))
	{
		delete _is;
		throw rts2core::ConnError ("Failed parsing reply");
	}

	delete _is;
}

void
ConnNUT::getValue (const char *var, Rts2ValueFloat *value)
{
	float val;
	getVal (var, val);
	value->setValueFloat (val);
}


void
ConnNUT::getValue (const char *var, Rts2ValueInteger *value)
{
	int val;
	getVal (var, val);
	value->setValueInteger (val);
}


void
ConnNUT::getValue (const char *var, Rts2ValueString *value)
{
	std::istringstream *_is = NULL;

	sendData (std::string ("GET VAR ") + upsName
		+ " " + std::string (var) + std::string ("\n"));
	receiveData (&_is, 2, '\n');

	std::string var_str, ups_name, var_name;
	char ap;

	*_is >> var_str;

	if (var_str != "VAR")
	{
		delete _is;
		throw rts2core::ConnError ((std::string ("Error getting ") + var + " " + var_str).c_str ());
	}
	
	*_is >> ups_name >> var_name >> ap;
	if (!(_is->good () && ap == '"'))
	{
		delete _is;
		throw rts2core::ConnError ("Failed parsing reply");
	}

	std::string val;
	while (true)
	{
		std::string v;
		*_is >> v;
		if (_is->fail ())
		{
			delete _is;
			throw rts2core::ConnError ("Failed parsing reply");
		}
		if (v[v.length () - 1] == '"')
		{
			val += v.substr (0, v.length () - 1);
			value->setValueString (val);
			delete _is;
			return;
		}	
		val += v + ' ';
	}
}


int
NUT::processOption (int opt)
{
	char *p;
	switch (opt)
	{
		case 'n':
			p = strchr (optarg, '@');
			if (p == NULL)
			{
				logStream (MESSAGE_ERROR) << "cannot locate @ at -n parameter (" << optarg << ")" << sendLog;
				return -1;
			}
			*p = '\0';
			upsName = optarg;
			host = new HostString (p + 1, "3493");
			break;
		case OPT_MINBCHARGE:
			minbcharge->setValueCharArr (optarg);
			break;
		case OPT_MINBRUNTIM:
			mintimeleft->setValueCharArr (optarg);
			break;
		case OPT_MAXONBAT:
			maxonbattery->setValueCharArr (optarg);
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
	
	connNUT = new ConnNUT (this, host->getHostname (), host->getPort (), upsName);
	ret = connNUT->init ();
	if (ret)
		return ret;
	ret = info ();
	if (ret)
		return ret;
	
	connNUT->getValue ("ups.model", model);

	setIdleInfoInterval (10);
	return 0;
}


int
NUT::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	if (old_value == minbcharge || old_value == mintimeleft)
		return 0;
	return SensorWeather::setValue (old_value, new_value);
}


int
NUT::info ()
{
	try
	{
		connNUT->getValue ("ups.load", loadpct);
		connNUT->getValue ("battery.charge", bcharge);
		connNUT->getValue ("battery.runtime", bruntime);
		connNUT->getValue ("ups.status", upsstatus);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}

	// perform variable checks..
	
	if (bcharge->getValueFloat () < minbcharge->getValueFloat ())
	{
	 	logStream (MESSAGE_WARNING) << "battery charge too low: " << bcharge->getValueFloat () << " < " << minbcharge->getValueFloat () << sendLog;
		setWeatherTimeout (1200);
	}

	if (bruntime->getValueInteger () < mintimeleft->getValueInteger ())
	{
	 	logStream (MESSAGE_WARNING) << "minimal battery time too low: " << bruntime->getValueInteger () << " < " << mintimeleft->getValueInteger () << sendLog;
		setWeatherTimeout (1200);

	}

	// if there is any UPS error, set big timeout..
	if (!(upsstatus->getValue () == std::string("OL CHRG") || upsstatus->getValue () == std::string ("OB DISCHRG")
	  	|| upsstatus->getValue () == std::string ("OL") || upsstatus->getValue () == std::string ("OB")))
	{
		logStream (MESSAGE_WARNING) <<  "unknow status " << upsstatus->getValue () << sendLog;
		setWeatherTimeout (1200);
	}

	// we are online - increase onbatterytimeout
	if (upsstatus->getValue () == std::string ("OL CHRG") || upsstatus->getValue () == std::string ("OL"))
	{
		onbatterytimeout->setValueInteger ((int) getNow () + maxonbattery->getValueInteger ());
	}
	

	if (onbatterytimeout->getValueInteger () <= getNow ())
	{
		logStream (MESSAGE_WARNING) << "running for too long on battery" << sendLog;
		setWeatherTimeout (1200);
	}
	
	return SensorWeather::info ();
}


NUT::NUT (int argc, char **argv):SensorWeather (argc, argv)
{
	host = NULL;
	upsName = NULL;
	connNUT = NULL;

  	createValue (model, "ups.model", "UPS mode", false);
	createValue (loadpct, "ups.load", "UPS load", false);
	createValue (bcharge, "battery.charge", "battery charge", false);
	createValue (bruntime, "battery.runtime", "time left for on-UPS operations", false);

	createValue (onbatterytimeout, "on_battery", "Till this time we are allowed to run even on battery", false);

	createValue (upsstatus, "ups.status", "UPS status", false);

	createValue (minbcharge, "min_bcharge", "minimal battery charge for opening", false);
	minbcharge->setValueFloat (50);
	createValue (mintimeleft, "min_tleft", "minimal time left for UPS operation", false);
	mintimeleft->setValueInteger (1200);

	createValue (maxonbattery, "max_onbattery", "maximal time we are allowed to run on battery", false);
	maxonbattery->setValueInteger (60);

	addOption ('n', NULL, 1, "upsname@hostname[:port] of NUT");

	addOption (OPT_MINBCHARGE, "min-battery.charge", 1, "minimal battery charge (in % - 0 - 100) to declare good weather");
	addOption (OPT_MINBRUNTIM, "min-battery.runtime", 1, "minimal battery runtime (in seconde) to declare good weather");
	addOption (OPT_MAXONBAT, "max-onbattery", 1, "maximal time on battery before declaring bad weather. Default to 60 seconds.");
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
