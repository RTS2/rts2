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

			template <typename t> void getVal (const char *var, t val);
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


ConnNUT::ConnNUT (Rts2Block *_master, const char *_hostname, int _port, const char *_upsName)
:rts2core::ConnTCP (_master, _hostname, _port)
{
	upsName = std::string (upsName);
}


template <typename t> void
ConnNUT::getVal (const char *var, t val)
{
	std::istringstream *_is = NULL;

	sendData (std::string ("GET VAR ") + upsName
		+ std::string (var) + std::string ("\n"));
	receiveData (&_is, 2, '\n');

	std::string var_str, ups_name, var_name;

	*_is >> var_str >> ups_name >> var_name >> val;

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
	setIdleInfoInterval (10);
	return 0;
}


int
NUT::info ()
{
	try
	{
		connNUT->getValue ("ups.load", loadpct);
		connNUT->getValue ("battery.charge", bcharge);
		connNUT->getValue ("battery.runtime", bruntime);
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
/*	if (strcmp (status->getValue (), "ONLINE") && strcmp (status->getValue (), "ONBATT"))
	{
		logStream (MESSAGE_WARNING) <<  "unknow status " << status->getValue () << sendLog;
		setWeatherTimeout (1200);
	}*/
	
	return SensorWeather::info ();
}


NUT::NUT (int argc, char **argv):SensorWeather (argc, argv)
{
	host = NULL;
	upsName = NULL;
	connNUT = NULL;

//  	createValue (model, "model", "UPS mode", false);
	createValue (loadpct, "load", "UPS load", false);
	createValue (bcharge, "bcharge", "battery charge", false);
	createValue (bruntime, "bruntime", "time left for on-UPS operations", false);
//	createValue (status, "status", "UPS status", false);

	createValue (minbcharge, "min_bcharge", "minimal battery charge for opening", false);
	minbcharge->setValueFloat (50);
	createValue (mintimeleft, "min_tleft", "minimal time left for UPS operation", false);
	mintimeleft->setValueInteger (1200);

	addOption ('n', NULL, 1, "upsname@hostname[:port] of NUT");
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
