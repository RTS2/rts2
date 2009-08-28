/* 
 * Driver for Keithley 6485 picoAmpermeter.
 * Copyright (C) 2008-2009 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/error.h"

namespace rts2sensord
{

/**
 * Keithley SCPI.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Keithley:public Gpib
{
	private:
		void getGPIB (const char *buf, Rts2ValueDoubleStat *sval, rts2core::DoubleArray * val, rts2core::DoubleArray *times, int count);

		void waitOpc ();

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

void Keithley::getGPIB (const char *buf, Rts2ValueDoubleStat *sval, rts2core::DoubleArray * val, rts2core::DoubleArray *times, int count)
{
	int bsize = 10000;
	char *rbuf = new char[bsize];
	gpibWrite (buf);
	gpibRead (rbuf, bsize);
	// check if top contains valid header
	if (rbuf[0] != '#' || rbuf[1] != '0')
	{
		throw rts2core::Error ("Buffer does not start with #");
	}
	char *top = rbuf + 2;
	while (top - rbuf + 13 <= bsize)
	{
		float rval = *((float *) (top + 0));
		// fields are specified with FORMAT:ELEM, and are in READ,TIME,STAT order..
		rval *= 10e+12;
		sval->addValue (rval);
		val->addValue (rval);
		/*logStream (MESSAGE_DEBUG) << "data "
			<< *((float *) (top + 0)) << " "
			<< *((float *) (top + 4)) << " "
			<< *((float *) (top + 8)) << sendLog; */
		times->addValue (*((float *) (top + 4)));
		count--;
		top += 12;
	}
	logStream (MESSAGE_DEBUG) << "size " << sval->getNumMes () << " " << val->size () << " count " << count << sendLog;
	delete[]rbuf;
}

void Keithley::waitOpc ()
{
	int icount = 0;
	while (icount < countNum->getValueInteger ())
	{
		int opc;
		readInt ("*OPC?", opc);
		if (opc == 1)
			return;
		usleep (USEC_SEC / 100);
		icount++;
	}
	throw rts2core::Error ("waitOpc timeout");
}


Keithley::Keithley (int in_argc, char **in_argv):
Gpib (in_argc, in_argv)
{
	createValue (azero, "AZERO", "SYSTEM:AZERO value");
	createValue (scurrent, "CURRENT", "Measured current statistics", true, RTS2_VWHEN_BEFORE_END);
	createValue (current, "A_CURRENT", "Measured current", true, RTS2_VWHEN_BEFORE_END);
	createValue (meas_times, "MEAS_TIMES", "Measurement times (delta)", true, RTS2_VWHEN_BEFORE_END);
	createValue (countNum, "COUNT", "Number of measurements averaged", true);
	countNum->setValueInteger (100);
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
	try
	{
		gpibWrite ("TRIG:DEL 0");
		// start and setup measurements..
		gpibWrite ("*RST");
		gpibWrite ("TRIG:DEL 0");
		writeValue ("TRIG:COUNT", countNum);
		gpibWrite ("SENS:CURR:RANG:AUTO ON");
		gpibWrite ("SENS:CURR:NPLC 1");
		// gpibWrite ("SENS:CURR:RANG 2000");
		gpibWrite ("SYST:ZCH OFF");
		gpibWrite ("SYST:AZER:STAT OFF");
		writeValue ("TRAC:POIN", countNum);
		gpibWrite ("TRAC:CLE");
		gpibWrite ("TRAC:FEED:CONT NEXT");
		gpibWrite ("STAT:MEAS:ENAB 512");
		gpibWrite ("*SRE 1");
	
		// ask for Automatic ZERO
		readValue ("SYSTEM:AZERO?", azero);
	
		// start and setup measurements..
		gpibWrite ("*CLS");

		// set format..
		gpibWrite (":FORM:DATA SRE; ELEM READ,TIME,STAT ; BORD SWAP ; :TRAC:TST:FORM ABS");
		
		waitOpc ();
		// scale current
		scurrent->clearStat ();
		current->clear ();
		meas_times->clear ();
		waitOpc ();
	}
	catch (rts2core::Error er)
	{
	 	logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}


int
Keithley::initValues ()
{
	Rts2ValueString *model = new Rts2ValueString ("model");
	readValue ("*IDN?", model);
	addConstValue (model);
	return Gpib::initValues ();
}


int
Keithley::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	try
	{
		if (old_value == azero)
		{
			writeValue ("SYSTEM:AZERO", new_value);
			return 0;
		}
		if (old_value == countNum)
		{
			writeValue ("TRIG:COUNT", new_value);
			writeValue ("TRAC:POIN", new_value);
			gpibWrite ("TRAC:CLE");
			waitOpc ();
			return 0;
		}
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "cannot set " << new_value->getName () << " " << er << sendLog;
		return -2;
	}
	return Gpib::setValue (old_value, new_value);
}


int
Keithley::info ()
{
	try
	{
		// disable display
		waitOpc ();
		// scale current
		scurrent->clearStat ();
		current->clear ();
		meas_times->clear ();
		// start taking data
		gpibWrite ("INIT");
		int ret = Gpib::info ();
		if (ret)
			return ret;
		// now wait for SQR
		sleep (2);
		gpibWaitSRQ ();
		getGPIB ("TRAC:DATA?", scurrent, current, meas_times, countNum->getValueInteger ());
		gpibWrite ("TRAC:CLE");
		gpibWrite ("TRAC:FEED:CONT NEXT");
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}


int
main (int argc, char **argv)
{
	Keithley device = Keithley (argc, argv);
	return device.run ();
}
