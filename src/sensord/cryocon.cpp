#include "sensorgpib.h"

#include "../utils/error.h"

namespace rts2sensord
{

class Cryocon;

/**
 * Structure which holds information about input channel
 */
class Rts2ValueTempInput
{
	private:
		char chan;
	public:
		std::list < Rts2Value * >values;

		Rts2ValueTempInput (Cryocon * dev, char in_chan);

		char getChannel ()
		{
			return chan;
		}
};

/**
 * Structure which holds loop inforamtions
 */
class Rts2ValueLoop
{
	private:
		int loop;
	public:
		Rts2ValueSelection * source;
		Rts2ValueDouble *setpt;
		Rts2ValueSelection *type;
		Rts2ValueSelection *range;
		Rts2ValueBool *ramp;
		Rts2ValueDouble *rate;
		Rts2ValueDouble *pgain;
		Rts2ValueDouble *igain;
		Rts2ValueDouble *dgain;
		Rts2ValueDouble *htrread;
		Rts2ValueFloat *pmanual;

		Rts2ValueLoop (Cryocon * dev, int in_loop);

		std::list < Rts2Value * >values;

		int getLoop ()
		{
			return loop;
		}
};

class Cryocon:public Gpib
{
	void writeRead (const char *buf, Rts2Value * val);
	void writeRead (const char *subsystem, std::list < Rts2Value * >&vals, int prefix_num);

	void writeRead (const char *buf, Rts2ValueDouble * val);
	void writeRead (const char *buf, Rts2ValueFloat * val);
	void writeRead (const char *buf, Rts2ValueBool * val);
	void writeRead (const char *buf, Rts2ValueSelection * val);

	const char *getLoopVal (int l, Rts2Value * val);

	Rts2ValueTempInput *chans[4];
	Rts2ValueLoop *loops[2];

	std::list < Rts2Value * >systemList;

	Rts2ValueDouble *statTime;

	Rts2ValueDouble *amb;
	Rts2ValueFloat *htrread;
	Rts2ValueFloat *htrhst;
	Rts2ValueBool *heaterEnabled;
	protected:
		virtual int setValue (Rts2Value * oldValue, Rts2Value * newValue);

	public:
		Cryocon (int argc, char **argv);
		virtual ~ Cryocon (void);

		void createTempInputValue (Rts2ValueDouble ** val, char chan,
			const char *name, const char *desc);

		template < typename T > void createLoopValue (T * &val, int loop,
			const char *in_val_name,
			const char *in_desc,
			bool writeToFits = true)
		{
			char *n = new char[strlen (in_val_name) + 3];
			n[0] = '1' + loop;
			n[1] = '.';
			strcpy (n + 2, in_val_name);
			createValue (val, n, in_desc, writeToFits);
			delete[] n;
		}

		virtual int info ();

		virtual int commandAuthorized (Rts2Conn * conn);
};

};

using namespace rts2sensord;

Rts2ValueTempInput::Rts2ValueTempInput (Cryocon * dev,
char in_chan)
{
	chan = in_chan;
	// values are passed to dev, and device deletes them!
	Rts2ValueDouble *v;

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


Rts2ValueLoop::Rts2ValueLoop (Cryocon * dev, int in_loop)
{
	loop = in_loop;

	dev->createLoopValue (source, loop, "SOURCE", "Control lopp source input");
	values.push_back (source);

	// fill in values
	const char *sourceVals[] = { "CHA", "CHB", "CHC", "CHD", NULL };
	source->addSelVals (sourceVals);

	dev->createLoopValue (setpt, loop, "SETPT", "Control loop set point");
	values.push_back (setpt);

	dev->createLoopValue (type, loop, "TYPE", "Control loop control type");

	values.push_back (type);

	const char *typeVals[] =
		{ "Off", "PID", "MAN", "Table", "RampP", "RampT", NULL };
	type->addSelVals (typeVals);

	dev->createLoopValue (range, loop, "RANGE", "Control loop output range");
	values.push_back (range);

	const char *rangeVals[] =
	{
		"50W", "5.0W", "0.5W", "0.05W", "25W", "2.5W", "0.25W", "0.03W", "HTR",
		NULL
	};
	range->addSelVals (rangeVals);

	dev->createLoopValue (ramp, loop, "RAMP", "Control loop ramp status");
	values.push_back (ramp);

	dev->createLoopValue (rate, loop, "RATE", "Control loop ramp rate");
	values.push_back (rate);

	dev->createLoopValue (pgain, loop, "PGAIN",
		"Control loop proportional gain term");
	values.push_back (pgain);

	dev->createLoopValue (igain, loop, "IGAIN",
		"Control loop integral gain term");
	values.push_back (igain);

	dev->createLoopValue (dgain, loop, "DGAIN",
		"Control loop derivative gain term");
	values.push_back (dgain);

	dev->createLoopValue (htrread, loop, "HTRREAD",
		"Control loop output current");
	values.push_back (htrread);

	dev->createLoopValue (pmanual, loop, "PMANUAL",
		"Control loop manual power output setting");
	values.push_back (pmanual);
}


void Cryocon::writeRead (const char *buf, Rts2Value * val)
{
	switch (val->getValueType ())
	{
		case RTS2_VALUE_DOUBLE:
			writeRead (buf, (Rts2ValueDouble *) val);
			return;
		case RTS2_VALUE_FLOAT:
			writeRead (buf, (Rts2ValueFloat *) val);
			return;
		case RTS2_VALUE_BOOL:
			writeRead (buf, (Rts2ValueBool *) val);
			return;
		case RTS2_VALUE_SELECTION:
			writeRead (buf, (Rts2ValueSelection *) val);
			return;
		default:
			logStream (MESSAGE_ERROR) << "Do not know how to read value " << val->
				getName () << " of type " << val->getValueType () << sendLog;
			throw rts2core::Error ("unknow value type");
	}
}


void Cryocon::writeRead (const char *subsystem, std::list < Rts2Value * >&vals, int prefix_num)
{
	char rb[500];
	char *retTop;
	char *retEnd;
	std::list < Rts2Value * >::iterator iter;

	strcpy (rb, subsystem);

	for (iter = vals.begin (); iter != vals.end (); iter++)
	{
		if (iter != vals.begin ())
			strcat (rb, ";");
		strcat (rb, (*iter)->getName ().c_str () + prefix_num);
		strcat (rb, "?");
	}
	gpibWriteRead (rb, rb, 500);
	// spit reply and set values..
	retTop = rb;
	for (iter = vals.begin (); iter != vals.end (); iter++)
	{
		retEnd = retTop;
		if (*retEnd == '\0')
		{
			logStream (MESSAGE_ERROR) << "Cannot find reply for value " <<
				(*iter)->getName () << sendLog;
			throw rts2core::Error ("Cannot find reply");
		}
		while (*retEnd && *retEnd != ';')
		{
			if (*retEnd == ' ')
				*retEnd = '\0';
			retEnd++;
		}
		*retEnd = '\0';

		int ret = (*iter)->setValueString (retTop);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Error when setting value " <<
				(*iter)->getName () << " to " << retTop << sendLog;
			throw rts2core::Error ("Error when setting value");
		}
		retTop = retEnd + 1;
	}
}


void Cryocon::writeRead (const char *buf, Rts2ValueDouble * val)
{
	char rb[50];
	gpibWriteRead (buf, rb, 50);
	val->setValueDouble (atof (rb));
}


void Cryocon::writeRead (const char *buf, Rts2ValueFloat * val)
{
	char rb[50];
	gpibWriteRead (buf, rb, 50);
	val->setValueFloat (atof (rb));
}


void Cryocon::writeRead (const char *buf, Rts2ValueBool * val)
{
	char rb[50];
	gpibWriteRead (buf, rb, 50);
	val->setValueBool (!strncmp (rb, "ON", 2));
}


void Cryocon::writeRead (const char *buf, Rts2ValueSelection * val)
{
	char rb[50];
	gpibWriteRead (buf, rb, 50);
	val->setSelIndex (rb);
}


const char *
Cryocon::getLoopVal (int l, Rts2Value * val)
{
	static char buf[100];
	strcpy (buf, "LOOP ");
	// run info for loops
	buf[5] = '1' + l;
	buf[6] = ':';
	strcpy (buf + 7, val->getName ().c_str () + 2);
	return buf;
}


int
Cryocon::setValue (Rts2Value * oldValue, Rts2Value * newValue)
{
	try
	{
		for (int l = 0; l < 2; l++)
		{
			for (std::list < Rts2Value * >::iterator iter = loops[l]->values.begin (); iter != loops[l]->values.end (); iter++)
			{
				Rts2Value *val = *iter;
				if (oldValue == val)
				{
					writeValue (getLoopVal (l, val), newValue);
					return 0;
				}
			}
		}
		if (oldValue == heaterEnabled)
		{
			if (((Rts2ValueBool *) newValue)->getValueBool ())
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
		chans[i] = new Rts2ValueTempInput (this, 'A' + i);
	}

	for (i = 0; i < 2; i++)
	{
		loops[i] = new Rts2ValueLoop (this, i);
	}

	createValue (statTime, "STATTIME", "time for which statistic was collected",
		true);

	createValue (amb, "AMBIENT", "cryocon ambient temperature", true);
	createValue (htrread, "HTRREAD", "Heater read back current", true);
	createValue (htrhst, "HTRHST", "Heater heat sink temperature", true);

	createValue (heaterEnabled, "HEATER", "Heater enabled/disabled", true);

	systemList.push_back (amb);
	systemList.push_back (htrread);
	systemList.push_back (htrhst);
}


Cryocon::~Cryocon (void)
{
}


void
Cryocon::createTempInputValue (Rts2ValueDouble ** val, char chan,
const char *name,
const char *desc)
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


int
Cryocon::info ()
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
			writeRead (buf, chans[i]->values, 2);
		}
		strcpy (buf, "LOOP ");
		buf[7] = '\0';
		// run info for loops
		for (i = 0; i < 2; i++)
		{
			buf[5] = '1' + i;
			buf[6] = ':';
			writeRead (buf, loops[i]->values, 2);
		}
		writeRead ("STATS:TIME?", statTime);
		statTime->setValueDouble (statTime->getValueDouble () * 60.0);
		writeRead ("SYSTEM:", systemList, 0);
		writeRead ("CONTROL?", heaterEnabled);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::info ();
}


int
Cryocon::commandAuthorized (Rts2Conn * conn)
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


int
main (int argc, char **argv)
{
	Cryocon device = Cryocon (argc, argv);
	return device.run ();
}
