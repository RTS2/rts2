/* 
 * Driver for Lakeshore 325 temperature controller.
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

#include "sensorgpib.h"

#include "error.h"

namespace rts2sensord
{

class Lakeshore;

/**
 * Values for a single channel.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TempChannel:public std::map < const char*, std::list <rts2core::Value *> >
{
	public:
		TempChannel () {};
		/**
		 * Returns iterator to array holding value.
		 */
		std::map <const char *, std::list <rts2core::Value *> >::iterator findValue (rts2core::Value *value);
};

/**
 * Driver for Lakeshore 325 temperature controller.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Lakeshore:public Gpib
{
	public:
		Lakeshore (int argc, char **argv);
		virtual ~ Lakeshore (void);

		virtual int info ();

	protected:
		virtual int init ();

		virtual int setValue (rts2core::Value * oldValue, rts2core::Value * newValue);

	private:
		TempChannel *temps[2];
		TempChannel *loops[2];

		rts2core::ValueBool *tunest;

		template < typename T> rts2core::Value *tempValue (T *&val, const char *prefix, char chan, const char *_info, bool writeToFits, uint32_t flags = 0)
		{
			std::ostringstream _os_n, _os_i;
			_os_n << chan << "." << prefix;
			_os_i << chan << " " << _info;
			createValue (val, _os_n.str ().c_str (), _os_i.str ().c_str (), writeToFits, flags);
			return val;
		}

		void changeTempValue (char chan, std::map <const char *, std::list <rts2core::Value *> >::iterator it, rts2core::Value *oldValue, rts2core::Value *newValue);
};

}

using namespace rts2sensord;

std::map <const char *, std::list <rts2core::Value *> >::iterator TempChannel::findValue (rts2core::Value *value)
{
	for (std::map <const char *, std::list <rts2core::Value *> >::iterator iter = begin (); iter != end (); iter++)
	{
		std::list <rts2core::Value *>::iterator it_val = std::find (iter->second.begin (), iter->second.end (), value);
		if (it_val != iter->second.end ())
			return iter;
	}
	return end ();
}

Lakeshore::Lakeshore (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	int i;
	char chan;
	rts2core::ValueDouble *vd;
	rts2core::ValueInteger *vi;
	rts2core::ValueBool *vb;
	rts2core::ValueSelection *vs;

	for (i = 0, chan = 'A'; i < 2; i++, chan++)
	{
		temps[i] = new TempChannel ();

		(*(temps[i]))["CRDG"].push_back (tempValue (vd, "TEMP", chan, "channel temperature", true));
		(*(temps[i]))["INCRV"].push_back (tempValue (vi, "INCRV", chan, "channel input curve number", true, RTS2_VALUE_WRITABLE));
		(*(temps[i]))["RDGST"].push_back (tempValue (vi, "STATUS", chan, "channel input reading status", true));
		(*(temps[i]))["SRDG"].push_back (tempValue (vd, "SENSOR", chan, "channel input sensor value", true));

		(*(temps[i]))["FILTER"].push_back (tempValue (vb, "FILTER_ACTIVE", chan, "channel filter function state", false, RTS2_VALUE_WRITABLE));
		(*(temps[i]))["FILTER"].push_back (tempValue (vi, "FILTER_POINTS", chan, "channel filter number of data points used for the filtering function. Valid range 2-64.", false, RTS2_VALUE_WRITABLE));
		(*(temps[i]))["FILTER"].push_back (tempValue (vi, "FILTER_WINDOW", chan, "channel filtering window. Valid range 1 - 10", false, RTS2_VALUE_WRITABLE));

		(*(temps[i]))["INTYPE"].push_back (tempValue (vs, "INTYPE_SENSOR", chan, "channel input sensor type", false, RTS2_VALUE_WRITABLE));
		vs->addSelVal ("Silicon diode");
		vs->addSelVal ("GaAlAs diode");
		vs->addSelVal ("100 Ohm platinum/250");
		vs->addSelVal ("100 Ohm platinum/500");
		vs->addSelVal ("1000 Ohm platinum");
		vs->addSelVal ("NTC RTD");
		vs->addSelVal ("Thermocouple 25 mV");
		vs->addSelVal ("Thermocouple 50 mV");
		vs->addSelVal ("2.5 V,1 mA");
		vs->addSelVal ("7.5 V,1 mA");
		(*(temps[i]))["INTYPE"].push_back (tempValue (vb, "INTYPE_COMPENSATION", chan, "channel input sensor compensation", false, RTS2_VALUE_WRITABLE));
	}

	for (i = 0, chan = '1'; i < 2; i++, chan++)
	{
		loops[i] = new TempChannel ();

		(*(loops[i]))["SETP"].push_back (tempValue (vd, "SETPOINT", chan, "control loop temperature setpoint", true, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["HTR"].push_back (tempValue (vd, "HEATER", chan, "control loop heater output", true));

		(*(loops[i]))["RANGE"].push_back (tempValue (vs, "HEATER_RANGE", chan, "control loop heater range", false, RTS2_VALUE_WRITABLE));
		vs->addSelVal ("Off");
		vs->addSelVal ("Low (2.5 W)");
		vs->addSelVal ("High (25 W)");

		(*(loops[i]))["HTRRES"].push_back (tempValue (vs, "HEATER_RESISTANCE", chan, "control loop heater output", false, RTS2_VALUE_WRITABLE));
		vs->addSelVal ("25 Ohm");
		vs->addSelVal ("50 Ohm");

		(*(loops[i]))["MOUT"].push_back (tempValue (vd, "MOUT", chan, "control loop manual heater power", false, RTS2_VALUE_WRITABLE));

		(*(loops[i]))["PID"].push_back (tempValue (vd, "PID_P", chan, "control loop PID Proportional (gain)", false, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["PID"].push_back (tempValue (vd, "PID_I", chan, "control loop PID Integral (reset)", false, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["PID"].push_back (tempValue (vd, "PID_D", chan, "control loop PID Derivative (rate)", false, RTS2_VALUE_WRITABLE));

		(*(loops[i]))["RAMP"].push_back (tempValue (vb, "RAMP_ACTIVE", chan, "control loop setpoint ramping", false, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["RAMP"].push_back (tempValue (vd, "RAMP_RATE", chan, "control loop ramping rate", false, RTS2_VALUE_WRITABLE));

		(*(loops[i]))["RAMPST"].push_back (tempValue (vb, "RAMP_STATUS", chan, "control loop ramping status", false, RTS2_VALUE_WRITABLE));

	}

	createValue (tunest, "TUNEST", "if either Loop 1 or Loop 2 is actively tuning.", true);
}


Lakeshore::~Lakeshore (void)
{
	for (int i = 0; i < 2; i++)
	{
		delete temps[i];
		temps[i] = NULL;

		delete loops[i];
		loops[i] = NULL;
	}
}

int Lakeshore::info ()
{
	try
	{
		int i;
		char chan;
		for (i = 0, chan = 'A'; i < 2; i++, chan++)
		{
			for (std::map <const char*, std::list <rts2core::Value*> >::iterator iter = temps[i]->begin (); iter != temps[i]->end (); iter++)
			{
				std::ostringstream _os;
				_os << iter->first << "? " << chan;

				char buf[100];
				gpibWriteRead (_os.str ().c_str (), buf, 100);
				std::vector <std::string> sc = SplitStr (std::string (buf), std::string (","));

				std::vector<std::string>::iterator iter_sc = sc.begin ();
				std::list <rts2core::Value*>::iterator iter_v = iter->second.begin ();
				for (; iter_sc != sc.end () && iter_v != iter->second.end (); iter_sc++, iter_v++)
				{
					(*iter_v)->setValueCharArr (iter_sc->c_str ());
				}
			}
		}

		for (i = 0, chan = '1'; i < 2; i++, chan++)
		{
			for (std::map <const char*, std::list <rts2core::Value*> >::iterator iter = loops[i]->begin (); iter != loops[i]->end (); iter++)
			{
				std::ostringstream _os;
				_os << iter->first << "? " << chan;

				char buf[100];
				gpibWriteRead (_os.str ().c_str (), buf, 100);
				std::vector <std::string> sc = SplitStr (std::string (buf), std::string (","));

				std::vector<std::string>::iterator iter_sc = sc.begin ();
				std::list <rts2core::Value*>::iterator iter_v = iter->second.begin ();
				for (; iter_sc != sc.end () && iter_v != iter->second.end (); iter_sc++, iter_v++)
				{
					(*iter_v)->setValueCharArr (iter_sc->c_str ());
				}
			}
		}

		readValue ("TUNEST?", tunest);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::info ();
}

int Lakeshore::init ()
{
	return Gpib::init ();
}

int Lakeshore::setValue (rts2core::Value * oldValue, rts2core::Value * newValue)
{
	try
	{
		int i;
		char chan;
		for (i = 0, chan = 'A'; i < 2; i++, chan++)
		{
			std::map <const char *, std::list <rts2core::Value *> >::iterator it = temps[i]->findValue (oldValue);
			if (it != temps[i]->end ())
			{
				changeTempValue (chan, it, oldValue, newValue);
				return 0;
			}
		}
		for (i = 0, chan = '1'; i < 2; i++, chan++)
		{
			std::map <const char *, std::list <rts2core::Value *> >::iterator it = loops[i]->findValue (oldValue);
			if (it != loops[i]->end ())
			{
				changeTempValue (chan, it, oldValue, newValue);
				return 0;
			}
		}
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -2;
	}
	return Gpib::setValue (oldValue, newValue);
}

void Lakeshore::changeTempValue (char chan, std::map <const char *, std::list <rts2core::Value *> >::iterator it, rts2core::Value *oldValue, rts2core::Value *newValue)
{
	std::ostringstream _os;
	_os << it->first << " " << chan;
	_os << std::fixed;
	for (std::list <rts2core::Value *>::iterator it_val = it->second.begin (); it_val != it->second.end (); it_val++)
	{
		_os << ",";
		if (*it_val == oldValue)
			_os << newValue->getValueFloat ();
		else
			_os << (*it_val)->getValueFloat ();
	}
	std::cout << _os.str () << std::endl;
	gpibWrite (_os.str ().c_str ());
}

int main (int argc, char **argv)
{
	Lakeshore device (argc, argv);
	return device.run ();
}
