/* 
 * Driver for Lakeshore 340 temperature controller.
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 * Copyright (C) 2011 Christopher Klein, UC Berkeley
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

// Global constants
const int NUM_LOOP = 2;
const int NUM_CHAN = 10;
const char *channames[NUM_CHAN] = {"A", "B", "C1", "C2", "C3", "C4", "D1", "D2", "D3", "D4"};

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
 * Driver for Lakeshore 340 temperature controller. Based on
 * http://www.sal.wisc.edu/whirc/archive/public/datasheets/lakeshore/340_Manual.pdf
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
		TempChannel *temps[NUM_CHAN];
		TempChannel *loops[NUM_LOOP];
        
		// query heater output
		rts2core::ValueFloat *htr; // heater output in percent
        
		// query heater status
		rts2core::ValueInteger *htrst; // heater error code
        
		// configure heater range
		rts2core::ValueInteger *range; // heater range (0-5)
        
		template < typename T > rts2core::Value *tempValue (T *&val, const char *prefix, const char *chan, const char *_info, bool writeToFits, uint32_t flags = 0)
		{
			std::ostringstream _os_n, _os_i;
			_os_n << chan << "." << prefix;
			_os_i << chan << " " << _info;
			createValue (val, _os_n.str ().c_str (), _os_i.str (), writeToFits, flags);
			return val;
		}

		template < typename T > rts2core::Value *loopValue (T * &val, const char *name, const char *desc, int i, bool writeToFits = true, int32_t valueFlags = 0, int queCondition = 0)
		{
			std::ostringstream _os_n, _os_i;
			_os_n << name << "_" << i;
			_os_i << desc << " " << i;
			createValue (val, _os_n.str ().c_str (), _os_i.str (), writeToFits, valueFlags, queCondition);
			return val;
		}

		void readChannelValues (const char *pn, TempChannel *pv);

		void changeChannelValue (const char *chan, std::map <const char *, std::list <rts2core::Value *> >::iterator it, rts2core::Value *oldValue, rts2core::Value *newValue);
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

/*
 * Edits by John Capone on 08/26/11 (jicapone@astro.umd.edu)
 */
Lakeshore::Lakeshore (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	int i;
	const char *chan;
	rts2core::ValueDouble *vd;
	rts2core::ValueInteger *vi;
	rts2core::ValueBool *vb;
	rts2core::ValueSelection *vs;

	for (i = 0, chan = channames[0]; i < NUM_CHAN; i++, chan = channames[i])
	{
		temps[i] = new TempChannel ();

		(*(temps[i]))["CRDG"].push_back (tempValue (vd, "TEMP", chan, "[C] channel temperature", true));
		(*(temps[i]))["KRDG"].push_back (tempValue (vd, "TEMP_K", chan, "[K] channel temperature in deg K", true));
		(*(temps[i]))["FILTER"].push_back (tempValue (vb, "FILTER_ACTIVE", chan, "channel filter function state", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE));
		(*(temps[i]))["FILTER"].push_back (tempValue (vi, "FILTER_POINTS", chan, "channel filter number of data points used for the filtering function. Valid range 2-64.", false, RTS2_VALUE_WRITABLE));
		(*(temps[i]))["FILTER"].push_back (tempValue (vi, "FILTER_WINDOW", chan, "channel filtering window. Valid range 1 - 10", false, RTS2_VALUE_WRITABLE));
	}
    
	for (i = 0; i < NUM_LOOP; i++)
	{
		int j = i + 1;
		loops[i] = new TempChannel ();
		(*(loops[i]))["CFILT"].push_back (loopValue (vb, "CFILT", "control filter, loop", j, true, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE));

		(*(loops[i]))["CLIMIT"].push_back (loopValue (vd, "CLIMIT_SP", "setpoint limit value, loop", j, true, RTS2_VALUE_WRITABLE));
       		(*(loops[i]))["CLIMIT"].push_back (loopValue (vd, "CLIMIT_PS", "max. positive slope, loop", j, true, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["CLIMIT"].push_back (loopValue (vd, "CLIMIT_NS", "max. negative slop, loop", j, true, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["CLIMIT"].push_back (loopValue (vi, "CLIMIT_MC", "max. current (1=0.25A - 4=2.0A), loop", j, true, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["CLIMIT"].push_back (loopValue (vi, "CLIMIT_MR", "max. loop 1 heater range (0-5)", j, true, RTS2_VALUE_WRITABLE));

		(*(loops[i]))["CMODE"].push_back (loopValue (vs, "CMODE", "control mode, loop", j, true, RTS2_VALUE_WRITABLE));
		vs->addSelVal ("Manual PID");
		vs->addSelVal ("Zone");
		vs->addSelVal ("Open Loop");
		vs->addSelVal ("AutoTune PID");
		vs->addSelVal ("AutoTune PI");
		vs->addSelVal ("AutoTune P");

		(*(loops[i]))["CSET"].push_back (loopValue (vs, "CSET_IN", "input, loop", j, true, RTS2_VALUE_WRITABLE));
		vs->addSelVal ("D1");
		vs->addSelVal ("D2");
		vs->addSelVal ("C1");
		vs->addSelVal ("C2");
		vs->addSelVal ("C3");
		vs->addSelVal ("C4");
		vs->addSelVal ("D1");
		vs->addSelVal ("D2");
		vs->addSelVal ("D3");
		vs->addSelVal ("D4");

		(*(loops[i]))["CSET"].push_back (loopValue (vs, "CSET_UN", "units, loop", j, true, RTS2_VALUE_WRITABLE));
		vs->addSelVal ("Kelvin");
		vs->addSelVal ("Celsius");
		vs->addSelVal ("sensor units");

		(*(loops[i]))["CSET"].push_back (loopValue (vb, "CSET_ST", "state, loop", j, true, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE));
		(*(loops[i]))["CSET"].push_back (loopValue (vb, "CSET_IS", "state after powerup, loop", j, true, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE));

		(*(loops[i]))["MOUT"].push_back (loopValue (vd, "MOUT", "[%] control loop manual output", j, true, RTS2_VALUE_WRITABLE));

		(*(loops[i]))["PID"].push_back (loopValue (vd, "PID_P", "PID p value, loop", j, true, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["PID"].push_back (loopValue (vd, "PID_I", "PID i value, loop", j, true, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["PID"].push_back (loopValue (vi, "PID_D", "PID d value, loop", j, true, RTS2_VALUE_WRITABLE));

		(*(loops[i]))["RAMP"].push_back (loopValue (vb, "RAMP_ST", "ramping status, loop", j, true, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE));
		(*(loops[i]))["RAMP"].push_back (loopValue (vd, "RAMP_RT", "[K/min] ramping rate, loop", j, false, RTS2_VALUE_WRITABLE));
		(*(loops[i]))["SETP"].push_back (loopValue (vd, "SETP", "[some units] setpoint, loop", j, true, RTS2_VALUE_WRITABLE));
	}
    
	createValue(htr, "HTR", "query heater output", true);
	createValue(htrst, "HTRST", "query heater status", false);
	createValue(range, "RANGE", "heater range (0-5)", true, RTS2_VALUE_WRITABLE);
}


Lakeshore::~Lakeshore (void)
{
	for (int i = 0; i < NUM_CHAN; i++)
	{
		delete temps[i];
		temps[i] = NULL;
	}
}

/*
 * Edits by John Capone on 08/26/11 (jicapone@astro.umd.edu)
 */
int Lakeshore::info ()
{
	try
	{
		int i;
		const char *chan;
		for (i = 0, chan = channames[0]; i < NUM_CHAN; i++, chan = channames[i])
		{
			readChannelValues (chan, temps[i]);
		}
		// get control loop parameters

		char lname[2];
		lname[1] = '\0';
		for (i = 0; i < NUM_LOOP; i++)
		{
			lname[0] = '1' + i;
			readChannelValues (lname, loops[i]);
		}
		readValue ("HTR?", htr);
		readValue ("HTRST?", htrst);
		readValue ("RANGE?", range);
	}
	catch (rts2core::Error &er)
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

/*
 * Edits by John Capone on 08/26/11 (jicapone@astro.umd.edu)
 */
int Lakeshore::setValue (rts2core::Value * oldValue, rts2core::Value * newValue)
{
	try
	{
		int i;
		const char *chan;
		std::map <const char *, std::list <rts2core::Value *> >::iterator it;

		if (oldValue == range)
		{
			std::ostringstream os;
			os << "RANGE " << newValue->getDisplayValue ();
			gpibWrite (os.str ().c_str ());
		}
		for (i = 0, chan = channames[0]; i < NUM_CHAN; i++, chan = channames[i])
		{
			it = temps[i]->findValue (oldValue);
			if (it != temps[i]->end ())
			{
				changeChannelValue (chan, it, oldValue, newValue);
				return 0;
			}
		}
		char lname[2];
		lname[1] = '\0';
		for (i = 0; i < NUM_LOOP; i++)
		{
			lname[0] = '1' + i;
			it = loops[i]->findValue (oldValue);
			if (it != loops[i]->end ())
			{
				changeChannelValue (lname, it, oldValue, newValue);
				return 0;
			}
		}
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -2;
	}
	return Gpib::setValue (oldValue, newValue);
}

void Lakeshore::readChannelValues (const char *pn, TempChannel *pv)
{
	for (std::map <const char*, std::list <rts2core::Value*> >::iterator iter = pv->begin (); iter != pv->end (); iter++)
	{
		std::ostringstream _os;
		_os << iter->first << "? " << pn;

		char buf[100];
		gpibWriteRead (_os.str ().c_str (), buf, 100);
		std::vector <std::string> sc = SplitStr (std::string (buf), std::string (","));

		std::vector<std::string>::iterator iter_sc = sc.begin ();
		std::list <rts2core::Value*>::iterator iter_v = iter->second.begin ();
		for (; iter_sc != sc.end () && iter_v != iter->second.end (); iter_sc++, iter_v++)
		{
			if ((*iter_v)->getValueBaseType () == RTS2_VALUE_SELECTION && (*iter_v)->getName () != "CSET_IN_1" && (*iter_v)->getName () != "CSET_IN_2")
				(*iter_v)->setValueInteger (atoi (iter_sc->c_str ()) - 1);
			else
				(*iter_v)->setValueCharArr (iter_sc->c_str ());
		}
	}
}

void Lakeshore::changeChannelValue (const char *chan, std::map <const char *, std::list <rts2core::Value *> >::iterator it, rts2core::Value *oldValue, rts2core::Value *newValue)
{
	std::ostringstream _os;
	_os << it->first << " " << chan;
	_os << std::fixed;
	for (std::list <rts2core::Value *>::iterator vit = it->second.begin (); vit != it->second.end (); vit++)
	{
		_os << ",";
		if ((*vit)->getName () == "CSET_IN_1" || (*vit)->getName () == "CSET_IN_2")
		{
			if (*vit == oldValue)
				_os << newValue->getDisplayValue ();
			else
				_os << (*vit)->getDisplayValue ();
		}
		else
		{
			rts2core::Value *nv;
			if (*vit == oldValue)
				nv = newValue;
			else
				nv = *vit;
			if ((*vit)->getValueBaseType () == RTS2_VALUE_SELECTION)
				_os << nv->getValueInteger () + 1;
			else if ((*vit)->getValueBaseType () == RTS2_VALUE_BOOL)
				_os << (nv->getValueInteger () ? "1" : "0");
			else
				_os << nv->getDisplayValue ();
		}
	}
	gpibWrite (_os.str ().c_str ());
}

int main (int argc, char **argv)
{
	Lakeshore device (argc, argv);
	return device.run ();
}
