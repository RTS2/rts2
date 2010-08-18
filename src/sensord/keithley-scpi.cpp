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
	public:
		Keithley (int argc, char **argv);
		virtual ~ Keithley (void);

		virtual void setFullBopState (int new_state);

		virtual int info ();

	protected:
		virtual int processOption (int opt);

		virtual int init ();
		virtual int initValues ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		virtual bool canCallInfoFromTimer () { return false; }

	private:
		void getGPIB (const char *buf, Rts2ValueDoubleStat *sval, rts2core::DoubleArray * val, rts2core::DoubleArray *times, int count);

		void waitOpc ();
		void readErrors ();

		Rts2ValueBool *azero;

		// triggering mode
		Rts2ValueSelection *triggerMode;

		// current statistics and value
		Rts2ValueDoubleStat *scurrent;
		rts2core::DoubleArray *current;
		rts2core::DoubleArray *meas_times;

		Rts2ValueInteger *countNum;
		Rts2ValueFloat *nplc;

		int groupNumber;
};

}

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
		rval *= 1e+12;
		sval->addValue (rval);
		val->addValue (rval);
		times->addValue (*((float *) (top + 4)));
		count--;
		top += 12;
	}
	logStream (MESSAGE_DEBUG) << "received " << sval->getNumMes () << " readings." << sendLog;
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

void Keithley::readErrors ()
{
	int count = 0;
	readInt ("SYST:ERR:COUN?", count);
	for (; count >= 0; count--)
	{
		char rb[400];
		gpibWriteRead ("SYST:ERR?", rb, 200);
		char *sep = strchr (rb, '\n');
		if (sep)
			*sep = '\0';
		if (atoi (rb) != 0)
			logStream (MESSAGE_ERROR) << "error " << rb << sendLog;
	}
	gpibWrite ("TRAC:CLE");
}

Keithley::Keithley (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	createValue (azero, "AZERO", "SYSTEM:AZERO value", true, RTS2_VALUE_WRITABLE);

	createValue (triggerMode, "trigger_mode", "triggering mode", true, RTS2_VALUE_WRITABLE);
	triggerMode->addSelVal ("before exposure");
	triggerMode->addSelVal ("block move, before exposure");
	triggerMode->addSelVal ("manual");
	triggerMode->setValueInteger (0);

	createValue (countNum, "COUNT", "[counts] number of measurements averaged", true, RTS2_VALUE_WRITABLE);
	countNum->setValueInteger (100);

	createValue (nplc, "MEAS_NPLC", "Time of each measurment. In Hz multiples (1 = 1/60 sec)", true, RTS2_VALUE_WRITABLE);
	nplc->setValueFloat (1.0);

	groupNumber = 0;

	addOption ('g', NULL, 1, "FITS extension group number");
}

Keithley::~Keithley (void)
{
}

int Keithley::processOption (int opt)
{
	switch (opt)
	{
		case 'g':
			groupNumber = atoi (optarg);
			break;
		default:
			return Gpib::processOption (opt);
	}
	return 0;
}

int Keithley::init ()
{
	int ret = Gpib::init ();
	if (ret)
		return ret;

	createValue (meas_times, "MEAS_TIMES", "measurement times (delta)", true, RTS2_VWHEN_TRIGGERED | RTS2_WR_GROUP_NUMBER(groupNumber));
	createValue (scurrent, "CURRENT", "[pA] measured current statistics", true, RTS2_VWHEN_TRIGGERED | RTS2_WR_GROUP_NUMBER(groupNumber));
	createValue (current, "A_CURRENT", "[pA] measured current", true, RTS2_VWHEN_TRIGGERED | RTS2_WR_GROUP_NUMBER(groupNumber));

	try
	{
		// start and setup measurements..
		gpibWrite ("*RST");
		// start and setup measurements..
		gpibWrite ("*CLS");
		gpibWrite ("*SRE 0");

		gpibWrite (":FORM:ELEM READ,TIME,STAT");
		gpibWrite (":TRAC:CLE");
		gpibWrite (":TRAC:FEED SENS");
		gpibWrite (":TRAC:FEED:CONT NEV");
		gpibWrite (":STAT:PRES");
		devClear ();
		waitOpc ();
		writeValue (":TRIG:COUN", countNum);
//		sleep (1);
		writeValue (":TRAC:POIN", countNum);
		writeValue (":SENS:CURR:NPLC", nplc);
		gpibWrite (":TRIG:DEL 0");
		gpibWrite (":TRAC:CLE");
		gpibWrite (":TRAC:FEED:CONT NEXT");
		gpibWrite (":SENS:CURR:RANG:AUTO ON");
		// gpibWrite ("SENS:CURR:RANG 2000");
		gpibWrite (":SYST:ZCH OFF");
		gpibWrite (":SYST:AZER:STAT OFF");
//		writeValue (":TRAC:POIN", countNum);
		gpibWrite (":TRAC:CLE");
		gpibWrite (":TRAC:FEED:CONT NEXT");
		gpibWrite (":STAT:MEAS:ENAB 512");
		gpibWrite ("*SRE 1");
	
		// ask for Automatic ZERO
		readValue (":SYSTEM:AZERO?", azero);
	
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
	if (triggerMode->getValueInteger () == 0)
	{
		maskState (BOP_TRIG_EXPOSE, BOP_TRIG_EXPOSE, "waiting for exposure");
	}
	return 0;
}

int Keithley::initValues ()
{
	Rts2ValueString *model = new Rts2ValueString ("model");
	readValue ("*IDN?", model);
	addConstValue (model);
	return Gpib::initValues ();
}

int Keithley::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == triggerMode)
	{
		switch (new_value->getValueInteger ())
		{
			case 0:
				maskState (BOP_TRIG_EXPOSE, BOP_TRIG_EXPOSE, "waiting for exposure trigger");
				break;
			case 1:
				maskState (BOP_TRIG_EXPOSE, 0, "end wait for exposure trigger - trigerring mode changed");
				break;
		}
		return 0;
	}
	try
	{
		if (old_value == azero)
		{
			writeValue ("SYSTEM:AZERO", new_value);
			return 0;
		}
		if (old_value == countNum)
		{
			devClear ();
			gpibWrite (":TRAC:FEED:CONT NEV");
			writeValue (":TRIG:COUN", new_value);
			writeValue (":TRAC:POIN", new_value);
			gpibWrite (":TRAC:CLE");
			gpibWrite (":TRAC:FEED:CONT NEXT");
			return 0;
		}
		if (old_value == nplc)
		{
			writeValue (":SENS:CURR:NPLC", new_value);
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

void Keithley::setFullBopState (int new_state)
{
	if (triggerMode->getValueInteger () == 0 || triggerMode->getValueInteger () == 1)
	{
		if ((new_state & BOP_WILL_EXPOSE) && !(getDeviceBopState () & BOP_WILL_EXPOSE))
		{
			if (triggerMode->getValueInteger () == 1)
				maskState (BOP_TRIG_EXPOSE | BOP_TEL_MOVE, BOP_TEL_MOVE, "end waiting for exposure trigger");
			else
				maskState (BOP_TRIG_EXPOSE, 0, "end waiting for exposure trigger");
			infoAll ();
			logStream (MESSAGE_DEBUG) << "infoAll triggered" << sendLog;
			if (triggerMode->getValueInteger () == 1)
				maskState (BOP_TEL_MOVE, 0, "measurement completed");
		}
		if (new_state & BOP_TEL_MOVE)
		{
			maskState (BOP_TRIG_EXPOSE, BOP_TRIG_EXPOSE, "waiting for exposure");
		}
	}
	Gpib::setFullBopState (new_state);
}

int Keithley::info ()
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
		// arbitary units!
		float expTi = countNum->getValueInteger () * nplc->getValueFloat ();
		if (expTi > 1500)
			settmo (100);
		else if (expTi > 700)
			settmo (30);
		else if (countNum->getValueInteger () > 200)
			settmo (10);
		gpibWrite ("INIT");
		int ret = Gpib::info ();
		if (ret)
			return ret;

		logStream (MESSAGE_DEBUG) << "trigger readout" << sendLog;
		// now wait for SQR
		gpibWaitSRQ ();
		sleep (1);
		getGPIB ("TRAC:DATA?", scurrent, current, meas_times, countNum->getValueInteger ());

		readErrors ();

		gpibWrite ("TRAC:FEED:CONT NEXT");
		if (countNum->getValueInteger () > 200)
			setTimeout (3);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

int main (int argc, char **argv)
{
	Keithley device = Keithley (argc, argv);
	return device.run ();
}
