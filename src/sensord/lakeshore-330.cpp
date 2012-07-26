/* 
 * Driver for Lakeshore 330 temperature controller.
 * Copyright (C) 2011 Petr Kubanek., Institute of Physics <kubanek@fzu.cz>
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
		virtual int initValues ();

		virtual int setValue (rts2core::Value * oldValue, rts2core::Value * newValue);

	private:
		TempChannel *temps[2];

		rts2core::ValueSelection *cchan;
		rts2core::ValueFloat *setp;
		rts2core::ValueInteger *heater;
		rts2core::ValueSelection *heaterStatus;
		rts2core::ValueBool *ramp;
		rts2core::ValueFloat *rampRate;
		rts2core::ValueBool *rampStatus;
		rts2core::ValueSelection *tunest;

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
	rts2core::ValueBool *vb;

	for (i = 0, chan = 'A'; i < 2; i++, chan++)
	{
		temps[i] = new TempChannel ();

		(*(temps[i]))["SDAT"].push_back (tempValue (vd, "TEMP", chan, "channel temperature", true));
		(*(temps[i]))["FILT"].push_back (tempValue (vb, "FILTER_ACTIVE", chan, "channel filter function state", false, RTS2_VALUE_WRITABLE));
	}

	createValue (cchan, "CCHAN", "control channel", true, RTS2_VALUE_WRITABLE);
	cchan->addSelVal ("A");
	cchan->addSelVal ("B");

	createValue (setp, "SETPOINT", "control loop temperature setpoint", true, RTS2_VALUE_WRITABLE);
	createValue (heater, "HEATER", "control loop heater output", true);
	createValue (heaterStatus, "HEATER_STATUS", "heater status", false, RTS2_VALUE_WRITABLE);
	heaterStatus->addSelVal ("off");
	heaterStatus->addSelVal ("low");
	heaterStatus->addSelVal ("medium");
	heaterStatus->addSelVal ("high");

	createValue (ramp, "RAMP_ACTIVE", "control loop setpoint ramping", false, RTS2_VALUE_WRITABLE);
	createValue (rampRate, "RAMP_RATE", "current ramp rate", false, RTS2_VALUE_WRITABLE);
	createValue (rampStatus, "RAMP_STATUS", "ramping status", false, RTS2_VALUE_WRITABLE);
	createValue (tunest, "TUNE", "autotuning status", true, RTS2_VALUE_WRITABLE);
	tunest->addSelVal ("Manual");
	tunest->addSelVal ("P");
	tunest->addSelVal ("PI");
	tunest->addSelVal ("PID");
	tunest->addSelVal ("Zone");
}


Lakeshore::~Lakeshore (void)
{
	for (int i = 0; i < 2; i++)
	{
		delete temps[i];
		temps[i] = NULL;
	}
}

int Lakeshore::info ()
{
	try
	{
		int i;
		char chan;
		for (i = 1, chan = 'B'; i >= 0; i--, chan--)
		{
			gpibWrite ((std::string ("SCHN ") + chan).c_str ());
			// time to change channel..
			sleep (1);
			for (std::map <const char*, std::list <rts2core::Value*> >::iterator iter = temps[i]->begin (); iter != temps[i]->end (); iter++)
			{
				std::ostringstream _os;
				_os << iter->first << "?";

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
		
		readValue ("CCHN?", cchan);
		readValue ("SETP?", setp);
		readValue ("HEAT?", heater);
		readValue ("RANG?", heaterStatus);
		readValue ("RAMP?", ramp);
		readValue ("RAMPR?", rampRate);
		readValue ("RAMPS?", rampStatus);
		readValue ("TUNE?", tunest);
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
	int ret = Gpib::init ();
	if (ret)
		return ret;
	gpibWrite ("TERM 3");
	return 0;
}

int Lakeshore::initValues ()
{
	rts2core::ValueString *model = new rts2core::ValueString ("model");
	readValue ("*IDN?", model);
	addConstValue (model);
	return Gpib::initValues ();
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
		if (oldValue == cchan)
		{
			writeValue ("CCHN", newValue);
			return 0;
		}
		else if (oldValue == setp)
		{
			writeValue ("SETP", newValue);
			return 0;
		}
		else if (oldValue == heaterStatus)
		{
			writeValue ("RANG", newValue);
			return 0;
		}
		else if (oldValue == ramp)
		{
			writeValue ("RAMP", (rts2core::ValueInteger *) newValue);
			return 0;
		}
		else if (oldValue == heaterStatus)
		{
			writeValue ("RANG", newValue);
			return 0;
		}
		else if (oldValue == rampRate)
		{
			writeValue ("RAMPR", newValue);
			return 0;
		}
		else if (oldValue == tunest)
		{
			writeValue ("tune", (rts2core::ValueInteger *) newValue);
			return 0;
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
	_os << newValue->getValueFloat ();
	gpibWrite (_os.str ().c_str ());
}

int main (int argc, char **argv)
{
	Lakeshore device (argc, argv);
	return device.run ();
}
