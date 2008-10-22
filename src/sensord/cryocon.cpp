#include "sensorgpib.h"

class Rts2DevSensorCryocon;

/**
 * Structure which holds information about input channel
 */
class Rts2ValueTempInput
{
	private:
		char chan;
	public:
		std::list < Rts2Value * >values;

		Rts2ValueTempInput (Rts2DevSensorCryocon * dev, char in_chan);

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

		Rts2ValueLoop (Rts2DevSensorCryocon * dev, int in_loop);

		std::list < Rts2Value * >values;

		int getLoop ()
		{
			return loop;
		}
};

class Rts2DevSensorCryocon:public Rts2DevSensorGpib
{
	int write (const char *buf, const char *newVal);

	int writeRead (const char *buf, Rts2Value * val);
	int writeRead (const char *subsystem, std::list < Rts2Value * >&vals,
		int prefix_num);

	int writeRead (const char *buf, Rts2ValueDouble * val);
	int writeRead (const char *buf, Rts2ValueFloat * val);
	int writeRead (const char *buf, Rts2ValueBool * val);
	int writeRead (const char *buf, Rts2ValueSelection * val);

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
		Rts2DevSensorCryocon (int argc, char **argv);
		virtual ~ Rts2DevSensorCryocon (void);

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

Rts2ValueTempInput::Rts2ValueTempInput (Rts2DevSensorCryocon * dev,
char in_chan)
{
	chan = in_chan;
	// values are passed to dev, and device deletes them!
	Rts2ValueDouble *v;

	dev->createTempInputValue (&v, chan, "TEMP",
		"cryocon temperature from channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "MINIMUM",
		"minimum temperature on channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "MAXIMUM",
		"maximum temperature on channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "VARIANCE",
		"temperature variance of channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "SLOPE", "slope of channel ");
	values.push_back (v);

	dev->createTempInputValue (&v, chan, "OFFSET", "temp offset of channel ");
	values.push_back (v);
}


Rts2ValueLoop::Rts2ValueLoop (Rts2DevSensorCryocon * dev, int in_loop)
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


int
Rts2DevSensorCryocon::write (const char *buf, const char *newVal)
{
	int ret;
	char *vbuf = new char[strlen (buf) + strlen (newVal) + 2];
	strcpy (vbuf, buf);
	strcat (vbuf, " ");
	strcat (vbuf, newVal);
	ret = gpibWrite (vbuf);
	delete[]vbuf;
	return ret;
}


int
Rts2DevSensorCryocon::writeRead (const char *buf, Rts2Value * val)
{
	switch (val->getValueType ())
	{
		case RTS2_VALUE_DOUBLE:
			return writeRead (buf, (Rts2ValueDouble *) val);
		case RTS2_VALUE_FLOAT:
			return writeRead (buf, (Rts2ValueFloat *) val);
		case RTS2_VALUE_BOOL:
			return writeRead (buf, (Rts2ValueBool *) val);
		case RTS2_VALUE_SELECTION:
			return writeRead (buf, (Rts2ValueSelection *) val);
		default:
			logStream (MESSAGE_ERROR) << "Do not know how to read value " << val->
				getName () << " of type " << val->getValueType () << sendLog;
	}
	return -1;
}


int
Rts2DevSensorCryocon::writeRead (const char *subsystem, std::list < Rts2Value * >&vals, int prefix_num)
{
	char rb[500];
	char *retTop;
	char *retEnd;
	std::list < Rts2Value * >::iterator iter;
	int ret;

	strcpy (rb, subsystem);

	for (iter = vals.begin (); iter != vals.end (); iter++)
	{
		if (iter != vals.begin ())
			strcat (rb, ";");
		strcat (rb, (*iter)->getName ().c_str () + prefix_num);
		strcat (rb, "?");
	}
	ret = gpibWriteRead (rb, rb, 500);
	if (ret)
		return ret;
	// spit reply and set values..
	retTop = rb;
	for (iter = vals.begin (); iter != vals.end (); iter++)
	{
		retEnd = retTop;
		if (*retEnd == '\0')
		{
			logStream (MESSAGE_ERROR) << "Cannot find reply for value " <<
				(*iter)->getName () << sendLog;
			return -1;
		}
		while (*retEnd && *retEnd != ';')
		{
			if (*retEnd == ' ')
				*retEnd = '\0';
			retEnd++;
		}
		*retEnd = '\0';

		ret = (*iter)->setValueString (retTop);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Error when setting value " <<
				(*iter)->getName () << " to " << retTop << sendLog;
			return ret;
		}
		retTop = retEnd + 1;
	}
	return 0;
}


int
Rts2DevSensorCryocon::writeRead (const char *buf, Rts2ValueDouble * val)
{
	char rb[50];
	int ret = gpibWriteRead (buf, rb);
	if (ret)
		return ret;
	val->setValueDouble (atof (rb));
	return 0;
}


int
Rts2DevSensorCryocon::writeRead (const char *buf, Rts2ValueFloat * val)
{
	char rb[50];
	int ret = gpibWriteRead (buf, rb);
	if (ret)
		return ret;
	val->setValueFloat (atof (rb));
	return 0;
}


int
Rts2DevSensorCryocon::writeRead (const char *buf, Rts2ValueBool * val)
{
	char rb[50];
	int ret = gpibWriteRead (buf, rb);
	if (ret)
		return ret;
	val->setValueBool (!strncmp (rb, "ON", 2));
	return 0;
}


int
Rts2DevSensorCryocon::writeRead (const char *buf, Rts2ValueSelection * val)
{
	char rb[50];
	int ret = gpibWriteRead (buf, rb);
	if (ret)
		return ret;
	return val->setSelIndex (rb);
}


const char *
Rts2DevSensorCryocon::getLoopVal (int l, Rts2Value * val)
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
Rts2DevSensorCryocon::setValue (Rts2Value * oldValue, Rts2Value * newValue)
{
	for (int l = 0; l < 2; l++)
		for (std::list < Rts2Value * >::iterator iter = loops[l]->values.begin ();
		iter != loops[l]->values.end (); iter++)
	{
		Rts2Value *val = *iter;
		if (oldValue == val)
			return write (getLoopVal (l, val), newValue->getDisplayValue ());
	}
	if (oldValue == heaterEnabled)
	{
		if (((Rts2ValueBool *) newValue)->getValueBool ())
			return gpibWrite ("CONTROL");
		return gpibWrite ("STOP");
	}
	return Rts2DevSensorGpib::setValue (oldValue, newValue);
}


Rts2DevSensorCryocon::Rts2DevSensorCryocon (int in_argc, char **in_argv):
Rts2DevSensorGpib (in_argc, in_argv)
{
	int i;

	setPad (12);

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


Rts2DevSensorCryocon::~Rts2DevSensorCryocon (void)
{
}


void
Rts2DevSensorCryocon::createTempInputValue (Rts2ValueDouble ** val, char chan,
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
Rts2DevSensorCryocon::info ()
{
	int ret;
	char buf[50] = "INPUT ";
	int i;
	buf[8] = '\0';
	for (i = 0; i < 4; i++)
	{
		buf[6] = chans[i]->getChannel ();
		buf[7] = ':';
		ret = writeRead (buf, chans[i]->values, 2);
		if (ret)
			return ret;
	}
	strcpy (buf, "LOOP ");
	buf[7] = '\0';
	// run info for loops
	for (i = 0; i < 2; i++)
	{
		buf[5] = '1' + i;
		buf[6] = ':';
		ret = writeRead (buf, loops[i]->values, 2);
		if (ret)
			return ret;
	}
	ret = writeRead ("STATS:TIME?", statTime);
	if (ret)
		return ret;
	statTime->setValueDouble (statTime->getValueDouble () * 60.0);
	ret = writeRead ("SYSTEM:", systemList, 0);
	if (ret)
		return ret;
	ret = writeRead ("CONTROL?", heaterEnabled);
	if (ret)
		return ret;
	return Rts2DevSensorGpib::info ();
}


int
Rts2DevSensorCryocon::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("control"))
	{
		return gpibWrite ("CONTROL");
	}
	else if (conn->isCommand ("stop"))
	{
		return gpibWrite ("STOP");
	}

	return Rts2DevSensorGpib::commandAuthorized (conn);
}


int
main (int argc, char **argv)
{
	Rts2DevSensorCryocon device = Rts2DevSensorCryocon (argc, argv);
	return device.run ();
}
