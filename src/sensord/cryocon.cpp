/* 
 * Driver for Cryocon temperature controller.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#define EVENT_TIMESERIE_COLLECT   RTS2_LOCAL_EVENT + 1002

namespace rts2sensord
{

class Cryocon;

/**
 * Structure which holds information about input channel.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueTempInput
{
	public:
		std::list < rts2core::Value * >values;

		ValueTempInput (Cryocon * dev, char in_chan);

		char getChannel () { return chan; }

	private:
		char chan;
};

/**
 * Structure which holds loop inforamtions.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueLoop
{
	public:
		rts2core::ValueSelection * source;
		rts2core::ValueDouble *setpt;
		rts2core::ValueSelection *type;
		rts2core::ValueSelection *range;
		rts2core::ValueBool *ramp;
		rts2core::ValueDouble *rate;
		rts2core::ValueDouble *pgain;
		rts2core::ValueDouble *igain;
		rts2core::ValueDouble *dgain;
		rts2core::ValueDouble *htrread;
		rts2core::ValueFloat *pmanual;

		ValueLoop (Cryocon * dev, int in_loop);

		std::list < rts2core::Value * >values;

		int getLoop ()
		{
			return loop;
		}
	private:
		int loop;
};

/**
 * Driver class for Cryocon temperature controller.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Cryocon:public Gpib
{
	public:
		Cryocon (int argc, char **argv);
		virtual ~ Cryocon (void);

		void createTempInputValue (rts2core::ValueDouble ** val, char chan, const char *name, const char *desc);

		template < typename T > void createLoopValue (T * &val, int loop, const char *in_val_name, const char *in_desc, bool writeToFits = true, uint32_t flags = 0)
		{
			char *n = new char[strlen (in_val_name) + 3];
			n[0] = '1' + loop;
			n[1] = '.';
			strcpy (n + 2, in_val_name);
			createValue (val, n, in_desc, writeToFits, flags);
			delete[] n;
		}

		virtual void postEvent (rts2core::Event *event);

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int initHardware ();

		virtual int info ();

		virtual int setValue (rts2core::Value * oldValue, rts2core::Value * newValue);

	private:
		const char *getLoopVal (int l, rts2core::Value * val);

		ValueTempInput *chans[4];
		ValueLoop *loops[2];

		std::list < rts2core::Value * >systemList;

		rts2core::ValueDouble *statTime;

		rts2core::ValueDouble *amb;
		rts2core::ValueFloat *htrread;
		rts2core::ValueFloat *htrhst;
		rts2core::ValueBool *heaterEnabled;

		rts2core::ValueDoubleTimeserie *ts[4];
};

};

using namespace rts2sensord;

ValueTempInput::ValueTempInput (Cryocon * dev, char in_chan)
{
	chan = in_chan;
	// values are passed to dev, and device deletes them!
	rts2core::ValueDouble *v;

	dev->createTempInputValue (&v, chan, "TEMP", "cryocon temperature from channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "MINIMUM", "minimum temperature on channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "MAXIMUM", "maximum temperature on channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "VARIANCE", "temperature variance of channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "SLOPE", "slope of channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "OFFSET", "temp offset of channel ");
	values.push_back (v);
}

ValueLoop::ValueLoop (Cryocon * dev, int in_loop)
{
	loop = in_loop;

	dev->createLoopValue (source, loop, "SOURCE", "Control loop source input", true, RTS2_VALUE_WRITABLE);
	values.push_back (source);

	// fill in values
	const char *sourceVals[] = { "CHA", "CHB", "CHC", "CHD", NULL };
	source->addSelVals (sourceVals);

	dev->createLoopValue (setpt, loop, "SETPT", "Control loop set point", true, RTS2_VALUE_WRITABLE);
	values.push_back (setpt);

	dev->createLoopValue (type, loop, "TYPE", "Control loop control type", true, RTS2_VALUE_WRITABLE);

	values.push_back (type);

	const char *typeVals[] =
		{ "Off", "PID", "MAN", "Table", "RampP", "RampT", NULL };
	type->addSelVals (typeVals);

	dev->createLoopValue (range, loop, "RANGE", "Control loop output range", true, RTS2_VALUE_WRITABLE);
	values.push_back (range);

	const char *rangeVals[] =
	{
		"50W", "5.0W", "0.5W", "0.05W", "25W", "2.5W", "0.25W", "0.03W", "HTR", NULL
	};
	range->addSelVals (rangeVals);

	dev->createLoopValue (ramp, loop, "RAMP", "Control loop ramp status");
	values.push_back (ramp);

	dev->createLoopValue (rate, loop, "RATE", "[K/min] Control loop ramp rate", true, RTS2_VALUE_WRITABLE);
	values.push_back (rate);

	dev->createLoopValue (pgain, loop, "PGAIN", "Control loop proportional gain term", true, RTS2_VALUE_WRITABLE);
	values.push_back (pgain);

	dev->createLoopValue (igain, loop, "IGAIN", "Control loop integral gain term", true, RTS2_VALUE_WRITABLE);
	values.push_back (igain);

	dev->createLoopValue (dgain, loop, "DGAIN", "Control loop derivative gain term", true, RTS2_VALUE_WRITABLE);
	values.push_back (dgain);

	dev->createLoopValue (htrread, loop, "HTRREAD", "Control loop output current");
	values.push_back (htrread);

	dev->createLoopValue (pmanual, loop, "PMANUAL", "Control loop manual power output setting", true, RTS2_VALUE_WRITABLE);
	values.push_back (pmanual);
}

const char * Cryocon::getLoopVal (int l, rts2core::Value * val)
{
	static char buf[100];
	strcpy (buf, "LOOP ");
	// run info for loops
	buf[5] = '1' + l;
	buf[6] = ':';
	strcpy (buf + 7, val->getName ().c_str () + 2);
	return buf;
}

int Cryocon::setValue (rts2core::Value * oldValue, rts2core::Value * newValue)
{
	try
	{
		for (int l = 0; l < 2; l++)
		{
			for (std::list < rts2core::Value * >::iterator iter = loops[l]->values.begin (); iter != loops[l]->values.end (); iter++)
			{
				rts2core::Value *val = *iter;
				if (oldValue == val)
				{
					writeValue (getLoopVal (l, val), newValue);
					return 0;
				}
			}
		}
		if (oldValue == heaterEnabled)
		{
			if (((rts2core::ValueBool *) newValue)->getValueBool ())
				gpibWrite ("CONTROL");
			else
				gpibWrite ("STOP");
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

Cryocon::Cryocon (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		chans[i] = new ValueTempInput (this, 'A' + i);
		std::ostringstream tsn;
		tsn << ('A' + i) << ".timeserie";
		createValue (ts[i], tsn.str ().c_str (), "temperature trending", false); 
	}

	for (i = 0; i < 2; i++)
	{
		loops[i] = new ValueLoop (this, i);
	}

	createValue (statTime, "STATTIME", "time for which statistic was collected", true);

	createValue (amb, "AMBIENT", "cryocon ambient temperature", true);
	createValue (htrread, "HTRREAD", "Heater read back current", true);
	createValue (htrhst, "HTRHST", "Heater heat sink temperature", true);

	createValue (heaterEnabled, "HEATER", "Heater enabled/disabled", true, RTS2_VALUE_WRITABLE);

	systemList.push_back (amb);
	systemList.push_back (htrread);
	systemList.push_back (htrhst);
}

Cryocon::~Cryocon (void)
{
}

void Cryocon::createTempInputValue (rts2core::ValueDouble ** val, char chan, const char *name, const char *desc)
{
	char *n = new char[strlen (name) + 3];
	n[0] = chan;
	n[1] = '.';
	strcpy (n + 2, name);
	char *d = new char[strlen (desc) + 2];
	strcpy (d, desc);
	strncat (d, &chan, 1);
	createValue (*val, n, d, true);
	delete[]n;
	delete[]d;
}

void Cryocon::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_TIMESERIE_COLLECT:
			for (int i = 0; i < 4; i++)
			{
				std::ostringstream os;
				os << "INPUT " << chans[i]->getChannel () << ":TEMP?";
				double v;
				readDouble (os.str ().c_str (), v);
				ts[i]->addValue (v, getNow (), 20);
				ts[i]->calculate ();
				sendValueAll (ts[i]);
			}

			addTimer (5, event);
			return;
	}
	Gpib::postEvent (event);
}

int Cryocon::initHardware ()
{
	int ret;
	ret = Gpib::initHardware ();
	if (ret)
		return ret;
	addTimer (1, new rts2core::Event (EVENT_TIMESERIE_COLLECT));
	return 0;
}

int Cryocon::info ()
{
	try
	{
		char buf[50] = "INPUT ";
		int i;
		buf[8] = '\0';
		for (i = 0; i < 4; i++)
		{
			buf[6] = chans[i]->getChannel ();
			buf[7] = ':';
			readValue (buf, chans[i]->values, 2);
		}
		strcpy (buf, "LOOP ");
		buf[7] = '\0';
		// run info for loops
		for (i = 0; i < 2; i++)
		{
			buf[5] = '1' + i;
			buf[6] = ':';
			readValue (buf, loops[i]->values, 2);
		}
		readValue ("STATS:TIME?", statTime);
		statTime->setValueDouble (statTime->getValueDouble () * 60.0);
		readValue ("SYSTEM:", systemList, 0);
		readValue ("CONTROL?", heaterEnabled);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::info ();
}

int Cryocon::commandAuthorized (rts2core::Connection * conn)
{
	try
	{
		if (conn->isCommand ("control"))
		{
			gpibWrite ("CONTROL");
			return 0;
		}
		else if (conn->isCommand ("stop"))
		{
			gpibWrite ("STOP");
			return 0;
		}
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_DEBUG) << er << sendLog;
		return -1;
	}

	return Gpib::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
	Cryocon device = Cryocon (argc, argv);
	return device.run ();
}
