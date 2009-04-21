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
		int getGPIB (const char *buf, Rts2ValueDoubleStat * val, int count, double scale = 1);

		int getGPIB (const char *buf, Rts2ValueBool * val);
		int setGPIB (const char *buf, Rts2ValueBool * val);

		int waitOpc ();

		Rts2ValueBool *azero;
		Rts2ValueDoubleStat *current;
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
	if (ret)
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
	if (ret)
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
	if (ret)
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
Keithley::getGPIB (const char *buf, Rts2ValueDoubleStat * val, int count, double scale)
{
	int ret;
	int bsize = 5000;
	char *rbuf = new char[bsize];
	ret = gpibWrite (buf);
	if (ret)
		return ret;
	ret = gpibRead (rbuf, bsize);
	if (ret)
		return ret;
	char *top = rbuf;
	char *start = rbuf;
	double uscale = 1e+12 / scale;
	while (top - rbuf < ibcnt)
	{
		if (*top == 'A')
		{
			top++;
			if (top - rbuf >= ibcnt)
				break;
			uscale = scale;
		}
		if (*top == ',')
		{
			double rval;
			*top = '\0';
			rval = atof (start);
			if (uscale == scale)
				val->addValue (rval * uscale);
			top++;
			start = top;
			uscale = 1e+12 / scale;
		}
		else
		{
			top++;
		}
	}
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
	if (ret)
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
	createValue (current, "CURRENT", "Measured current", true,
		RTS2_VWHEN_BEFORE_END);
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
	ret = gpibWrite ("TRIG:DEL 0");
	if (ret)
		return -1;
	// start and setup measurements..
	ret = gpibWrite ("*RST");
	if (ret)
		return ret;
	ret = gpibWrite ("TRIG:DEL 0");
	if (ret)
		return ret;
	ret = gpibWrite ("TRIG:COUNT 100");
	if (ret)
		return ret;
	ret = gpibWrite ("SENS:CURR:RANG:AUTO ON");
	if (ret)
		return ret;
	ret = gpibWrite ("SENS:CURR:NPLC 1");
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
	/*  ret = gpibWrite ("DISP:ENAB OFF");
	  if (ret)
		return ret;a */
	ret = gpibWrite ("TRAC:POIN 100");
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
	ret = gpibWrite ("*SRE 0");
	if (ret)
		return ret;

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
		return 0;
	}
	return Gpib::setValue (old_value, new_value);
}


int
Keithley::info ()
{
	int ret;
	// disable display
	//  char *buf;
	ret = getGPIB ("SYSTEM:AZERO?", azero);
	if (ret)
		return ret;
	// start and setup measurements..
	ret = gpibWrite ("*CLS");
	if (ret)
		return ret;
	ret = gpibWrite ("TRAC:POIN 100");
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
	ret = gpibWrite ("*SRE 0");
	if (ret)
		return ret;
	//asprintf (&buf, "*RST; TRIG:DEL 0; TRIG:COUNT %i; SENS:CURR:RANG:AUTO OFF; SENS:CURR:NPLC .01; SENS:CURR:RANG .002; SYST:ZCH OFF; SYST:AZER:STAT OFF; DISP:ENAB OFF; *CLS; TRAC:POIN %i; TRAC:CLE; TRAC:FEED:CONT NEXT; STAT:MEAS:ENAB 512; *SRE 1", countNum->getValueInteger (), countNum->getValueInteger ());
	//ret = gpibWrite (buf);
	//free (buf);
	ret = waitOpc ();
	if (ret)
		return ret;
	// scale current
	current->clearStat ();
	// start taking data
	ret = gpibWrite ("INIT");
	if (ret)
		return -1;
	// now wait for SQR
	//  ret = gpibWaitSRQ ();
	//  if (ret)
	//    return -1;
	//  sleep (1);
	/*  ret = gpibWrite ("DISP:ENAB ON");
	  if (ret)
		return ret; */
	ret = getGPIB ("TRAC:DATA?", current, countNum->getValueInteger (), 1e+12);
	if (ret)
		return ret;
	return Gpib::info ();
}


int
main (int argc, char **argv)
{
	Keithley device = Keithley (argc, argv);
	return device.run ();
}
