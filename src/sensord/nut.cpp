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

#include "connection/tcp.h"

#define OPT_MINBCHARGE   OPT_LOCAL + 221
#define OPT_MINBRUNTIM   OPT_LOCAL + 222
#define OPT_MAXONBAT     OPT_LOCAL + 223

namespace rts2sensord
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
			ConnNUT (rts2core::Block *_master, const char *_hostname, int _port, const char *_upsName);

			/**
			 * Call GET VAR command to receive variable from UPS.
			 *
			 * @param var    Variable name.
			 * @param value  Variable value.
			 *
			 * @throw ConnError
			 */
			void getValue (const char *var, rts2core::ValueFloat *value);

			void getValue (const char *var, rts2core::ValueInteger *value);
			void getValue (const char *var, rts2core::ValueString *value);

			template <typename t> void getVal (const char *var, t &val);
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

			rts2core::ValueString *model;

			rts2core::ValueFloat *loadpct;
			rts2core::ValueFloat *bcharge;
			rts2core::ValueInteger *bruntime;

			rts2core::ValueTime *onbatterytimeout;

			rts2core::ValueString *upsstatus;
			
			rts2core::ValueFloat *minbcharge;
			rts2core::ValueInteger *mintimeleft;
			rts2core::ValueInteger *maxonbattery;

		protected:
			virtual int processOption (int opt);
			virtual int info ();

			virtual int initHardware ();

		public:
			NUT (int argc, char **argv);
			virtual ~NUT (void);
	};

}


using namespace rts2sensord;


ConnNUT::ConnNUT (rts2core::Block *_master, const char *_hostname, int _port, const char *_upsName):rts2core::ConnTCP (_master, _hostname, _port)
{
	upsName = std::string (_upsName);
}

template <typename t> void ConnNUT::getVal (const char *var, t &val)
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
		if (var_str == "ERR")
		{
			throw rts2core::Error ("cannot retrieve value");
		}
		else
		{
			throw rts2core::ConnError (this, (std::string ("Error getting ") + var + " " + var_str).c_str ());
		}
	}
	
	*_is >> ups_name >> var_name >> ap >> val >> ap;
	if (!(_is->good () && ap == '"'))
	{
		delete _is;
		throw rts2core::ConnError (this, "Failed parsing reply");
	}

	delete _is;
}

void ConnNUT::getValue (const char *var, rts2core::ValueFloat *value)
{
	float val;
	getVal (var, val);
	value->setValueFloat (val);
}

void ConnNUT::getValue (const char *var, rts2core::ValueInteger *value)
{
	int val;
	getVal (var, val);
	value->setValueInteger (val);
}

void ConnNUT::getValue (const char *var, rts2core::ValueString *value)
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
		throw rts2core::ConnError (this, (std::string ("Error getting ") + var + " " + var_str).c_str ());
	}
	
	*_is >> ups_name >> var_name >> ap;
	if (!(_is->good () && ap == '"'))
	{
		delete _is;
		throw rts2core::ConnError (this, "Failed parsing reply");
	}

	std::string val;
	while (true)
	{
		std::string v;
		*_is >> v;
		if (_is->fail ())
		{
			delete _is;
			throw rts2core::ConnError (this, "Failed parsing reply");
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

int NUT::processOption (int opt)
{
	char *p;
	switch (opt)
	{
		case 'n':
			p = strchr (optarg, '@');
			if (p == NULL)
			{
				std::cerr << "cannot locate @ at -n parameter (" << optarg << ")" << std::endl;
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

int NUT::initHardware ()
{
  	int ret;
	int vi;
	float vf;

	if (upsName == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify UPS name with -n parameter" << sendLog;
		return -1;
	}
	
	connNUT = new ConnNUT (this, host->getHostname (), host->getPort (), upsName);
	ret = connNUT->init ();
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot connect to UPS " << upsName << " on host " << host->getHostname () << " port " << host->getPort () << sendLog;
		return ret;
	}

	try {
		connNUT->getVal ("ups.load", vf);
		createValue (loadpct, "ups.load", "UPS load", false);
	} catch (rts2core::Error er) {
		logStream (MESSAGE_DEBUG) << "ups.load value does not exist" << sendLog;
	}

	try {
		connNUT->getVal ("battery.charge", vf);
		createValue (bcharge, "battery.charge", "battery charge", false);
	} catch (rts2core::Error er) {
		logStream (MESSAGE_DEBUG) << "battery.charge value does not exist" << sendLog;
	}

	try {
		connNUT->getVal ("battery.runtime", vi);
		createValue (bruntime, "battery.runtime", "time left for on-UPS operations", false);
	} catch (rts2core::Error er) {
		logStream (MESSAGE_DEBUG) << "battery.runtime value does not exist" << sendLog;
	}

	connNUT->getValue ("ups.model", model);

	ret = info ();
	if (ret)
		return ret;

	setIdleInfoInterval (20);
	return 0;
}

int NUT::info ()
{
	try
	{
		if (loadpct)
			connNUT->getValue ("ups.load", loadpct);
		if (bcharge)
			connNUT->getValue ("battery.charge", bcharge);
		if (bruntime)
			connNUT->getValue ("battery.runtime", bruntime);
		connNUT->getValue ("ups.status", upsstatus);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}

	// perform variable checks..
	
	if (bcharge)
	{
		if (bcharge->getValueFloat () < minbcharge->getValueFloat ())
		{
		 	logStream (MESSAGE_WARNING) << "battery charge too low: " << bcharge->getValueFloat () << " < " << minbcharge->getValueFloat () << sendLog;
			setWeatherTimeout (1200, "low battery charge");
			valueError (bcharge);
		}
		else
		{
			valueGood (bcharge);
		}
	}

	if (bruntime)
	{
		if (bruntime->getValueInteger () < mintimeleft->getValueInteger ())
		{
		 	logStream (MESSAGE_WARNING) << "minimal battery time too low: " << bruntime->getValueInteger () << " < " << mintimeleft->getValueInteger () << sendLog;
			setWeatherTimeout (1200, "low minimal battery time");
			valueError (bruntime);
		}
		else
		{
			valueGood (bruntime);
		}
	}	

	// if there is any UPS error, set big timeout..
	if (!(upsstatus->getValue () == std::string("OL CHRG") || upsstatus->getValue () == std::string ("OB DISCHRG")
		|| upsstatus->getValue () == std::string ("OB DISCHRG LB")  	
		|| upsstatus->getValue () == std::string ("OL") || upsstatus->getValue () == std::string ("ALARM OL")	
		|| upsstatus->getValue () == std::string ("ALARM OL CHRG") || upsstatus->getValue () == std::string ("OB")
		|| upsstatus->getValue () == std::string ("LB OB") || upsstatus->getValue () == std::string ("LB OL")
		|| upsstatus->getValue () == std::string ("OL BOOST") || upsstatus->getValue () == std::string ("OL LB")))
	{
		logStream (MESSAGE_WARNING) <<  "unknow status " << upsstatus->getValue () << sendLog;
		setWeatherTimeout (1200, "unknow status");
		valueError (upsstatus);
	}
	else
	{
		valueGood (upsstatus);
	}

	// we are online - increase onbatterytimeout
	if (upsstatus->getValue () == std::string ("OL CHRG") || upsstatus->getValue () == std::string ("ALARM OL CHRG") || upsstatus->getValue () == std::string ("OL") || upsstatus->getValue () == std::string ("ALARM OL") || upsstatus->getValue () == std::string ("OL BOOST") || upsstatus->getValue () == std::string ("OL LB"))
	{
		onbatterytimeout->setValueInteger ((int) getNow () + maxonbattery->getValueInteger ());
	}
	

	if (onbatterytimeout->getValueInteger () <= getNow ())
	{
		logStream (MESSAGE_WARNING) << "running for too long on battery" << sendLog;
		setWeatherTimeout (1200, "running for too long on battery");
		valueError (onbatterytimeout);
	}
	else
	{
		valueGood (onbatterytimeout);
	}
	
	return SensorWeather::info ();
}

NUT::NUT (int argc, char **argv):SensorWeather (argc, argv)
{
	host = NULL;
	upsName = NULL;
	connNUT = NULL;

	loadpct = NULL;
	bcharge = NULL;
	bruntime = NULL;

  	createValue (model, "ups.model", "UPS mode", false);
	createValue (upsstatus, "ups.status", "UPS status", false);

	createValue (onbatterytimeout, "on_battery", "Till this time we are allowed to run even on battery", false);

	createValue (minbcharge, "min_bcharge", "minimal battery charge for opening", false, RTS2_VALUE_WRITABLE);
	minbcharge->setValueFloat (50);
	createValue (mintimeleft, "min_tleft", "minimal time left for UPS operation", false, RTS2_VALUE_WRITABLE);
	mintimeleft->setValueInteger (1200);

	createValue (maxonbattery, "max_onbattery", "maximal time we are allowed to run on battery", false, RTS2_VALUE_WRITABLE);
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

int main (int argc, char **argv)
{
	NUT device = NUT (argc, argv);
	return device.run ();
}
