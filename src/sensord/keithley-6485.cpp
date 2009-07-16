/* 
 * Sensor daemon for cloudsensor (mrakomer) by Martin Kakona
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

namespace rts2sensord
{

class Keithley:public Gpib
{
	private:
		int getGPIB (const char *buf, int &val);
		int getGPIB (const char *buf, Rts2ValueString * val);

		int getGPIB (double &rval);

		int getGPIB (const char *buf, Rts2ValueDouble * val);
		int getGPIB (const char *buf, Rts2ValueDoubleStat *sval, rts2core::DoubleArray * val, rts2core::DoubleArray *times, int count);

		int getGPIB (const char *buf, Rts2ValueBool * val);
		int setGPIB (const char *buf, Rts2ValueBool * val);
		int setGPIB (const char *buf, Rts2ValueInteger *val);

		int waitOpc ();

		Rts2ValueBool *azero;
		// current statistics and value
		Rts2ValueDoubleStat *scurrent;
		rts2core::DoubleArray *current;
		rts2core::DoubleArray *meas_times;

		Rts2ValueInteger *countNum;
	protected:
		virtual int init ();
		virtual int initValues ();
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
	public:
		Keithley (int argc, char **argv);
		virtual ~ Keithley (void);

		virtual int info ();
};

};

using namespace rts2sensord;

int
Keithley::getGPIB (const char *buf, int &val)
{
	int ret;
	char rb[200];
	ret = gpibWrite (buf);
	if (ret)
		return ret;
	ret = gpibRead (rb, 200);
	if (ret < 0)
		return ret;
	for (char *rb_top = rb; *rb_top; rb_top++)
	{
		if (*rb_top == '\n')
		{
			*rb_top = '\0';
			break;
		}
	}
	val = atol (rb);
	return 0;
}


int
Keithley::getGPIB (const char *buf, Rts2ValueString * val)
{
	int ret;
	char rb[200];
	ret = gpibWrite (buf);
	if (ret)
		return ret;
	ret = gpibRead (rb, 200);
	if (ret < 0)
		return ret;
	for (char *rb_top = rb; *rb_top; rb_top++)
	{
		if (*rb_top == '\n')
		{
			*rb_top = '\0';
			break;
		}
	}
	val->setValueString (rb);
	return 0;
}


int
Keithley::getGPIB (double &rval)
{
	int ret;
	char rb[200];
	ret = gpibRead (rb, 200);
	if (ret < 0)
		return ret;
	ret = sscanf (rb, "%lf", &rval);
	if (ret != 1)
		return -1;
	return 0;
}


int
Keithley::getGPIB (const char *buf, Rts2ValueDouble * val)
{
	int ret;
	double rval;
	ret = gpibWrite (buf);
	if (ret)
		return ret;
	ret = getGPIB (rval);
	if (ret)
		return ret;
	val->setValueDouble (rval);
	return 0;
}


int
Keithley::getGPIB (const char *buf, Rts2ValueDoubleStat *sval, rts2core::DoubleArray * val, rts2core::DoubleArray *times, int count)
{
	int ret;
	int bsize = 10000;
	char *rbuf = new char[bsize];
	ret = gpibWrite (buf);
	if (ret)
		return ret;
	bsize = gpibRead (rbuf, bsize);
	if (bsize < 0)
		return -1;
	logStream (MESSAGE_DEBUG) << "bsize " << bsize << " " << rbuf << sendLog;
	// check if top contains valid header
	if (rbuf[0] != '#' || rbuf[1] != '0')
	{
		logStream (MESSAGE_DEBUG) << "invalid header: " << rbuf[0] << " " << rbuf[1] << sendLog;
		return -1;
	}
	char *top = rbuf + 2;
	while (top - rbuf + 13 <= bsize)
	{
		float rval = *((float *) (top + 0));
		// fields are specified with FORMAT:ELEM, and are in READ,TIME,STAT order..
		rval *= 10e+12;
		sval->addValue (rval);
		val->addValue (rval);
		logStream (MESSAGE_DEBUG) << "data "
			<< *((float *) (top + 0)) << " "
			<< *((float *) (top + 4)) << " "
			<< *((float *) (top + 8)) << sendLog;
		times->addValue (*((float *) (top + 4)));
		count--;
		top += 12;
	}
	logStream (MESSAGE_DEBUG) << "size " << sval->getNumMes () << " " << val->size () << " count " << count << sendLog;
	delete[]rbuf;
	return 0;
}


int
Keithley::getGPIB (const char *buf, Rts2ValueBool * val)
{
	int ret;
	char rb[10];
	ret = gpibWrite (buf);
	if (ret)
		return ret;
	ret = gpibRead (rb, 10);
	if (ret < 0)
		return ret;
	if (atoi (rb) == 1)
		val->setValueBool (true);
	else
		val->setValueBool (false);
	return 0;
}


int
Keithley::setGPIB (const char *buf, Rts2ValueBool * val)
{
	char wr[strlen (buf) + 5];
	strcpy (wr, buf);
	if (val->getValueBool ())
		strcat (wr, " ON");
	else
		strcat (wr, " OFF");
	return gpibWrite (wr);
}


int
Keithley::setGPIB (const char *buf, Rts2ValueInteger * val)
{
	std::ostringstream _os;
	_os << buf << " " << val->getValueInteger ();
	return gpibWrite (_os.str ().c_str ());
}


int
Keithley::waitOpc ()
{
	int ret;
	int icount = 0;
	while (icount < countNum->getValueInteger ())
	{
		int opc;
		ret = getGPIB ("*OPC?", opc);
		if (ret)
			return -1;
		if (opc == 1)
			return 0;
		usleep (USEC_SEC / 100);
		icount++;
	}
	return -1;
}


Keithley::Keithley (int in_argc, char **in_argv):
Gpib (in_argc, in_argv)
{
	setPad (14);

	createValue (azero, "AZERO", "SYSTEM:AZERO value");
	createValue (scurrent, "CURRENT", "Measured current statistics", true, RTS2_VWHEN_BEFORE_END);
	createValue (current, "A_CURRENT", "Measured current", true, RTS2_VWHEN_BEFORE_END);
	createValue (meas_times, "MEAS_TIMES", "Measurement times (delta)", true, RTS2_VWHEN_BEFORE_END);
	createValue (countNum, "COUNT", "Number of measurements averaged", true);
	countNum->setValueInteger (7);
}


Keithley::~Keithley (void)
{
}


int
Keithley::init ()
{
	int ret = Gpib::init ();
	if (ret)
		return ret;
	// start and setup measurements..
	ret = gpibWrite ("*RST");
	if (ret)
		return ret;
	ret = gpibWrite ("TRIG:DEL 0");
	if (ret)
		return ret;
	ret = setGPIB ("TRIG:COUNT", countNum);
	if (ret)
		return ret;
	ret = gpibWrite ("SENS:CURR:RANG:AUTO ON");
	if (ret)
		return ret;
	ret = gpibWrite ("SENS:CURR:NPLC 1.0");
	if (ret)
		return ret;
	/*  ret = gpibWrite ("SENS:CURR:RANG 2000");
	  if (ret)
		return ret; */
	ret = gpibWrite ("SYST:ZCH OFF");
	if (ret)
		return ret;
	ret = gpibWrite ("SYST:AZER:STAT OFF");
	if (ret)
		return ret;
	ret = setGPIB ("TRAC:POIN", countNum);
	if (ret)
		return ret;
	ret = gpibWrite ("TRAC:CLE");
	if (ret)
		return ret;
	ret = gpibWrite ("TRAC:FEED:CONT NEXT");
	if (ret)
		return ret;
	ret = gpibWrite ("STAT:MEAS:ENAB 512");
	if (ret)
		return ret;
	ret = gpibWrite ("*SRE 1");
	if (ret)
		return ret;

	// ask for Automatic ZERO
	ret = getGPIB ("SYSTEM:AZERO?", azero);
	if (ret)
		return ret;
	// start and setup measurements..
//	ret = gpibWrite ("*CLS");
//	if (ret)
//		return ret;
	// set format..
	ret = gpibWrite (":FORM:DATA SRE; ELEM READ,TIME,STAT ; BORD SWAP ; :TRAC:TST:FORM ABS");
	if (ret)
		return ret;
	ret = waitOpc ();
	if (ret)
		return ret;
	// scale current
	scurrent->clearStat ();
	current->clear ();
	meas_times->clear ();
	// start taking data
	ret = waitOpc ();

	return ret;
}


int
Keithley::initValues ()
{
	int ret;
	Rts2ValueString *model = new Rts2ValueString ("model");
	ret = getGPIB ("*IDN?", model);
	if (ret)
		return -1;
	addConstValue (model);
	return Gpib::initValues ();
}


int
Keithley::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	int ret;
	if (old_value == azero)
	{
		ret = setGPIB ("SYSTEM:AZERO", (Rts2ValueBool *) new_value);
		if (ret)
			return -2;
		return 0;
	}
	if (old_value == countNum)
	{
		ret = setGPIB ("TRIG:COUNT", (Rts2ValueInteger *) new_value);
		if (ret)
			return -2;
		ret = setGPIB ("TRAC:POIN", (Rts2ValueInteger *) new_value);
		if (ret)
			return -2;
		ret = gpibWrite ("TRAC:CLE");
		if (ret)
			return ret;
		ret = waitOpc ();
		if (ret)
			return -2;
		return 0;
	}
	return Gpib::setValue (old_value, new_value);
}


int
Keithley::info ()
{
	int ret;
	// disable display
	ret = waitOpc ();
	if (ret)
		return ret;
	// scale current
	scurrent->clearStat ();
	current->clear ();
	meas_times->clear ();
	// start taking data
	logStream (MESSAGE_DEBUG) << "triggering beging" << sendLog;
	ret = gpibWrite (":INIT");
	if (ret)
		return -1;
	// update infotime..
	ret = Gpib::info ();
	if (ret)
		return ret;
	logStream (MESSAGE_DEBUG) << "triggering send and confirmed" << sendLog;
	// now wait for SQR
	sleep (2);
	ret = gpibWaitSRQ ();
	if (ret)
	    return -1;
	/*  ret = gpibWrite ("DISP:ENAB ON");
	  if (ret)
		return ret; */
	logStream (MESSAGE_DEBUG) << "data queried" << sendLog;
	ret = getGPIB ("TRAC:DATA?", scurrent, current, meas_times, countNum->getValueInteger ());
	if (ret)
		return ret;
	logStream (MESSAGE_DEBUG) << "data returned" << sendLog;
	ret = gpibWrite (":TRAC:CLE");
	if (ret)
		return ret;
	ret = gpibWrite (":TRAC:FEED:CONT NEXT");
	if (ret)
		return ret;
	return 0;
}


int
main (int argc, char **argv)
{
	Keithley device = Keithley (argc, argv);
	return device.run ();
}
