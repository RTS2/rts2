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
 * Driver for Lakeshore 340 temperature controller.
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
        
		// configure control filter
		rts2core::ValueBool *cflit[NUM_LOOP];  // |<on/off>|
        
		// configure control loop limit parameters
		rts2core::ValueFloat *climit_sp[NUM_LOOP]; // limit set point
		rts2core::ValueFloat *climit_ps[NUM_LOOP]; // limit max positive slope
		rts2core::ValueFloat *climit_ns[NUM_LOOP]; // limit max negative slope
		rts2core::ValueInteger *climit_mc[NUM_LOOP]; // limit max current (1=0.25A, 2=0.5A, 3=1.0A, 4=2.0A, 5=User)
		rts2core::ValueInteger *climit_mr[NUM_LOOP]; // limit max loop 1 heater range (0-5)
        
		// configure control loop mode
		rts2core::ValueInteger *cmode[NUM_LOOP]; // (1=manual PID, 2=zone, 3=open loop, 4=auto tune PID, 5=auto tune PI, 6=auto tune P)
        
		// configure control loop parameters
		rts2core::ValueSelection *cset_in[NUM_LOOP]; // which input (A-D4)
		rts2core::ValueInteger *cset_un[NUM_LOOP]; // setpoint units (1=kelvin, 2=celsius, 3=sensor units)
		rts2core::ValueBool *cset_st[NUM_LOOP]; // set control loop on/off
		rts2core::ValueBool *cset_is[NUM_LOOP]; // set control loop on/off after power-up
        
		// query heater output
		rts2core::ValueFloat *htr; // heater output in percent
        
		// query heater status
		rts2core::ValueInteger *htrst; // heater error code
        
		// configure control loop PID values
		rts2core::ValueFloat *pid_p[NUM_LOOP];    // value for P (0-1000, 0.1 resolution)
		rts2core::ValueFloat *pid_i[NUM_LOOP];    // value for I (0-1000, 0.1 resolution)
		rts2core::ValueInteger *pid_d[NUM_LOOP];  // value for D (0-1000, 1 resolution)
        
		// configure control loop ramp parameters
		rts2core::ValueBool *ramp_st[NUM_LOOP]; // set ramping on or off
		rts2core::ValueFloat *ramp_rt[NUM_LOOP]; // set ramp setpoint [kelvin/minute]
        
		// configure heater range
		rts2core::ValueInteger *range; // heater range (0-5)
        
		// configure control loop setpoint
		rts2core::ValueFloat *setp[NUM_LOOP]; // value of setpoint in given units
        
		template < typename T > rts2core::Value *tempValue (T *&val, const char *prefix, const char *chan, const char *_info, bool writeToFits, uint32_t flags = 0)
		{
			std::ostringstream _os_n, _os_i;
			_os_n << chan << "." << prefix;
			_os_i << chan << " " << _info;
			createValue (val, _os_n.str ().c_str (), _os_i.str (), writeToFits, flags);
			return val;
		}

		template < typename T > void channelValue (T * &val, const char *name, const char *desc, int i, bool writeToFits = true, int32_t valueFlags = 0, int queCondition = 0)
		{
			std::ostringstream _os_n, _os_i;
			_os_n << name << "_" << i;
			_os_i << desc << " " << i;
			createValue (val, _os_n.str ().c_str (), _os_i.str (), writeToFits, valueFlags, queCondition);
		}

		void changeTempValue (const char *chan, std::map <const char *, std::list <rts2core::Value *> >::iterator it, rts2core::Value *oldValue, rts2core::Value *newValue);
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
	rts2core::ValueBool *vb;
	rts2core::ValueSelection *vs;

	for (i = 0, chan = channames[0]; i < NUM_CHAN; i++, chan = channames[i])
	{
		temps[i] = new TempChannel ();

		(*(temps[i]))["SDAT"].push_back (tempValue (vd, "TEMP", chan, "channel temperature", true));
		(*(temps[i]))["FILT"].push_back (tempValue (vb, "FILTER_ACTIVE", chan, "channel filter function state", false, RTS2_VALUE_WRITABLE));
		(*(temps[i]))["INTYPE"].push_back (tempValue (vs, "INTYPE_SENSOR", chan, "channel input sensor type", false, RTS2_VALUE_WRITABLE));
		vs->addSelVal ("Silicon diode");
		vs->addSelVal ("Platinum");
		vs->addSelVal ("GaAlAs diode");
		vs->addSelVal ("Thermocouple");
	}
    
	for (i = 0; i < NUM_LOOP; i++)
	{
		int j = i + 1;
		channelValue (cflit[i], "CFLIT", "control filter, loop", j, true, RTS2_VALUE_WRITABLE);
    		channelValue (climit_sp[i], "CLIMIT_SP", "setpoint limit value, loop", j, true, RTS2_VALUE_WRITABLE);
       		channelValue (climit_ps[i], "CLIMIT_PS", "max. positive slope, loop", j, true, RTS2_VALUE_WRITABLE);
       		channelValue (climit_ns[i], "CLIMIT_NS", "max. negative slop, loop", j, true, RTS2_VALUE_WRITABLE);
        	channelValue (climit_mc[i], "CLIMIT_MC", "max. current (1=0.25A - 4=2.0A), loop", j, true, RTS2_VALUE_WRITABLE);
       		channelValue (climit_mr[i], "CLIMIT_MR", "max. loop 1 heater range (0-5)", j, true, RTS2_VALUE_WRITABLE);
        	channelValue (cmode[i], "CMODE", "control mode, loop", j, true, RTS2_VALUE_WRITABLE);
       		channelValue (cset_in[i], "CSET_IN", "input, loop", j, true, RTS2_VALUE_WRITABLE);
		for (int k = 0; k < NUM_CHAN; k++)
			cset_in[i]->addSelVal (channames[k]);
		channelValue (cset_un[i], "CSET_UN", "units, loop", j, true, RTS2_VALUE_WRITABLE);
		channelValue (cset_st[i], "CSET_ST", "state, loop", j, true, RTS2_VALUE_WRITABLE);
		channelValue (cset_is[i], "CSET_IS", "state after powerup, loop", j, true, RTS2_VALUE_WRITABLE);
		channelValue (pid_p[i], "PID_P", "PID p value, loop", j, true, RTS2_VALUE_WRITABLE);
		channelValue (pid_i[i], "PID_I", "PID i value, loop", j, true, RTS2_VALUE_WRITABLE);
		channelValue (pid_d[i], "PID_D", "PID d value, loop", j, true, RTS2_VALUE_WRITABLE);
		channelValue (ramp_st[i], "RAMP_ST", "ramping status, loop", j, true, RTS2_VALUE_WRITABLE);
		channelValue (ramp_rt[i], "RAMP_RT", "ramping rate, loop", j, false, RTS2_VALUE_WRITABLE);
		channelValue (setp[i], "SETP", "setpoint, loop", j, true, RTS2_VALUE_WRITABLE);
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
			gpibWrite ((std::string ("SCHN ") + chan).c_str ());
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
		
		/*        std::ostringstream tempS1;
        std::ostringstream tempS2;
        rts2core::ValueString *tempV = ValueString("tempV");
        for (i = 0; i < NUM_LOOP; i++)
        {
            
            tempS1 << "CFLIT? " << i;
            readValue (tempS1.str().c_str(), tempV);
            cflit[i]->setValueCharArr(tempV->getValue());
            tempS1.str(std::string());
            
            tempS1 << "CLIMIT? " << i;
            readValue (tempS1.str().c_str(), tempV);
            cflit[i]->setValueCharArr(tempV->getValue());
            tempS1.str(std::string());
            
	    }*/
        
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

/*
 * Edits by John Capone on 08/26/11 (jicapone@astro.umd.edu)
 */
int Lakeshore::setValue (rts2core::Value * oldValue, rts2core::Value * newValue)
{
	try
	{
		int i;
		const char *chan;
		for (i = 0, chan = channames[0]; i < NUM_CHAN; i++, chan = channames[i])
		{
			std::map <const char *, std::list <rts2core::Value *> >::iterator it = temps[i]->findValue (oldValue);
			if (it != temps[i]->end ())
			{
				changeTempValue (chan, it, oldValue, newValue);
				return 0;
			}
		}
        
		std::ostringstream tempS;
		rts2core::ValueString *tempV = new rts2core::ValueString ("tempV");
		int comLoop;
		for (comLoop = 1; comLoop <= 2; comLoop++)
		{
			if (oldValue == cflit[comLoop])
			{
				tempS << comLoop << ", " << (((rts2core::ValueBool *) newValue)->getValueBool () ? "ON" : "OFF");
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CFLIT", tempV);
				return 0;
			}
			else if (oldValue == climit_sp[comLoop])
			{
				tempS << comLoop << ", " << ((rts2core::ValueFloat *) newValue)->getDisplayValue () << ", " << climit_ps[comLoop]->getDisplayValue () << ", " << climit_ns[comLoop]->getDisplayValue () << ", " << climit_mc[comLoop]->getDisplayValue () << ", " << climit_mr[comLoop]->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CLIMIT", tempV);
				return 0;
			}
			else if (oldValue == climit_ps[comLoop])
			{
				tempS << comLoop << ", " << climit_sp[comLoop]->getDisplayValue () << ", " << ((rts2core::ValueFloat *) newValue)->getDisplayValue () << ", " << climit_ns[comLoop]->getDisplayValue () << ", " << climit_mc[comLoop]->getDisplayValue () << ", " << climit_mr[comLoop]->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CLIMIT", tempV);
				return 0;
			}
			else if (oldValue == climit_ns[comLoop])
			{
				tempS << comLoop << ", " << climit_sp[comLoop]->getDisplayValue () << ", " << climit_ps[comLoop]->getDisplayValue () << ", " << ((rts2core::ValueFloat *) newValue)->getDisplayValue () << ", " << climit_mc[comLoop]->getDisplayValue () << ", " << climit_mr[comLoop]->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CLIMIT", tempV);
				return 0;
			}
			else if (oldValue == climit_mc[comLoop])
			{
				tempS << comLoop << ", " << climit_sp[comLoop]->getDisplayValue () << ", " << climit_ps[comLoop]->getDisplayValue () << ", " << climit_ns[comLoop]->getDisplayValue () << ", " << ((rts2core::ValueInteger *) newValue)->getDisplayValue () << ", " << climit_mr[comLoop]->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CLIMIT", tempV);
				return 0;
			}
			else if (oldValue == climit_mr[comLoop])
			{
				tempS << comLoop << ", " << climit_sp[comLoop]->getDisplayValue () << ", " << climit_ps[comLoop]->getDisplayValue () << ", " << climit_ns[comLoop]->getDisplayValue () << ", " << climit_mc[comLoop]->getDisplayValue () << ", " << ((rts2core::ValueInteger *) newValue)->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CLIMIT", tempV);
				return 0;
			}
			else if (oldValue == cmode[comLoop])
			{
				tempS << comLoop << ", " << ((rts2core::ValueInteger *) newValue)->getDisplayValue();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CMODE", tempV);
				return 0;
			}
			else if (oldValue == cset_in[comLoop])
			{
				tempS << comLoop << ", " << ((rts2core::ValueString *) newValue)->getValue () << ", " << cset_un[comLoop]->getDisplayValue () << ", " << (((rts2core::ValueBool *) cset_st[comLoop])->getValueBool () ? "ON" : "OFF") << ", " << (((rts2core::ValueBool *) cset_is[comLoop])->getValueBool () ? "ON" : "OFF");
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CSET", tempV);
				return 0;
			}
			else if (oldValue == cset_un[comLoop])
			{
				tempS << comLoop << ", " << cset_in[comLoop]->getValue () << ", " << ((rts2core::ValueInteger *) newValue)->getDisplayValue () << ", " << (((rts2core::ValueBool *) cset_st[comLoop])->getValueBool () ? "ON" : "OFF") << ", " << (((rts2core::ValueBool *) cset_is[comLoop])->getValueBool () ? "ON" : "OFF");
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CSET", tempV);
				return 0;
			}
			else if (oldValue == cset_st[comLoop])
			{
				tempS << comLoop << ", " << cset_in[comLoop]->getValue () << ", " << cset_un[comLoop]->getDisplayValue () << ", " << (((rts2core::ValueBool *) newValue)->getValueBool () ? "ON" : "OFF") << ", " << (((rts2core::ValueBool *) cset_is[comLoop])->getValueBool () ? "ON" : "OFF");
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CSET", tempV);
				return 0;
			}
			else if (oldValue == cset_is[comLoop])
			{
				tempS << comLoop << ", " << cset_in[comLoop]->getValue () << ", " << cset_un[comLoop]->getDisplayValue () << ", " << (((rts2core::ValueBool *) cset_st[comLoop])->getValueBool () ? "ON" : "OFF") << ", " << (((rts2core::ValueBool *) newValue)->getValueBool () ? "ON" : "OFF");
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("CSET", tempV);
				return 0;
			}
			else if (oldValue == pid_p[comLoop])
			{
				tempS << comLoop << ", " << ((rts2core::ValueFloat *) newValue)->getDisplayValue () << ", " << pid_i[comLoop]->getDisplayValue () << ", " << pid_d[comLoop]->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("PID", tempV);
				return 0;
			}
			else if (oldValue == pid_i[comLoop])
			{
				tempS << comLoop << ", " << pid_p[comLoop]->getDisplayValue () << ", " << ((rts2core::ValueFloat *) newValue)->getDisplayValue () << ", " << pid_d[comLoop]->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("PID", tempV);
				return 0;
			}
			else if (oldValue == pid_d[comLoop])
			{
				tempS << comLoop << ", " << pid_p[comLoop]->getDisplayValue () << ", " << pid_i[comLoop]->getDisplayValue () << ", " << ((rts2core::ValueInteger *) newValue)->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("PID", tempV);
				return 0;
			}
			else if (oldValue == ramp_st[comLoop])
			{
				tempS << comLoop << ", " << (((rts2core::ValueBool *) newValue)->getValueBool () ? "ON" : "OFF") << ", " << ramp_rt[comLoop]->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("RAMP", tempV);
				return 0;
			}
			else if (oldValue == ramp_rt[comLoop])
			{
				tempS << comLoop << ", " << (((rts2core::ValueBool *) ramp_st[comLoop])->getValueBool () ? "ON" : "OFF") << ", " << ((rts2core::ValueFloat *) newValue)->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("RAMP", tempV);
				return 0;
			}
			else if (oldValue == range)
			{
				tempS << ((rts2core::ValueInteger *) newValue)->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("RANGE", tempV);
				return 0;
			}
			else if (oldValue == setp[comLoop])
			{
				tempS << comLoop << ", " << ((rts2core::ValueFloat *) newValue)->getDisplayValue ();
				tempV->setValueCharArr(tempS.str().c_str());
				writeValue("SETP", tempV);
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

void Lakeshore::changeTempValue (const char *chan, std::map <const char *, std::list <rts2core::Value *> >::iterator it, rts2core::Value *oldValue, rts2core::Value *newValue)
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
