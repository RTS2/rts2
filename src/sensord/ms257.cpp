/* 
 * Class for Newport MS-256 monochromator.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "newport.h"

class Rts2DevSensorMS257:public Rts2DevSensorNewport
{
	private:
		Rts2ValueDouble * msVer;
		Rts2ValueDouble *wavelenght;
		Rts2ValueInteger *slitA;
		Rts2ValueInteger *slitB;
		Rts2ValueInteger *slitC;
		Rts2ValueInteger *bandPass;
		Rts2ValueSelection *shutter;
		Rts2ValueInteger *filter1;
		//  Rts2ValueInteger *filter2;
		Rts2ValueInteger *msteps;
		Rts2ValueInteger *grat;

		int readRts2ValueFilter (const char *valueName, Rts2ValueInteger * val);

	protected:
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		Rts2DevSensorMS257 (int in_argc, char **in_argv);

		virtual int init ();
		virtual int info ();
};

int
Rts2DevSensorMS257::readRts2ValueFilter (const char *valueName,
Rts2ValueInteger * val)
{
	int ret;
	char *cval;
	char **pval = &cval;
	int iret;
	ret = readValue (valueName, pval);
	if (ret)
		return ret;
	if ((*cval != 'M' && *cval != 'A') || cval[1] != ':')
	{
		logStream (MESSAGE_ERROR) << "Unknow filter state: " << cval << sendLog;
		return -1;
	}
	iret = atoi (cval + 2);
	val->setValueInteger (iret);
	return 0;
}


Rts2DevSensorMS257::Rts2DevSensorMS257 (int in_argc, char **in_argv):
Rts2DevSensorNewport (in_argc, in_argv)
{
	createValue (msVer, "version", "version of MS257", false);
	createValue (wavelenght, "WAVELENG", "monochromator wavelength", true);

	createValue (slitA, "SLIT_A", "Width of the A slit in um", true);
	createValue (slitB, "SLIT_B", "Width of the B slit in um", true);
	createValue (slitC, "SLIT_C", "Width of the C slit in um", true);
	createValue (bandPass, "BANDPASS", "Automatic slit width in nm", true);

	createValue (shutter, "shutter", "Shutter settings", false);
	shutter->addSelVal ("SLOW");
	shutter->addSelVal ("FAST");
	shutter->addSelVal ("MANUAL");

	createValue (filter1, "FILT_1", "filter 1 position", true);
	//  createValue (filter2, "FILT_2", "filter 2 position", true);
	createValue (msteps, "MSTEPS",
		"Current grating position in terms of motor steps", true);
	createValue (grat, "GRATING", "Grating position", true);
}


int
Rts2DevSensorMS257::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == wavelenght)
	{
		return writeValue ("GW", new_value->getValueDouble (), '!');
	}
	if (old_value == slitA)
	{
		return writeValue ("SLITA", new_value->getValueInteger (), '!');
	}
	if (old_value == slitB)
	{
		return writeValue ("SLITB", new_value->getValueInteger (), '!');
	}
	if (old_value == slitC)
	{
		return writeValue ("SLITC", new_value->getValueInteger (), '!');
	}
	if (old_value == bandPass)
	{
		return writeValue ("BANDPASS", new_value->getValueInteger (), '=');
	}
	if (old_value == shutter)
	{
		char *shttypes[] = { "S", "F", "M" };
		if (new_value->getValueInteger () > 2
			|| new_value->getValueInteger () < 0)
			return -1;
		return writeValue ("SHTRTYPE", shttypes[new_value->getValueInteger ()],
			'=');
	}
	if (old_value == filter1)
	{
		return writeValue ("FILT1", new_value->getValueInteger (), '!');
	}
	/*  if (old_value == filter2)
		{
		  return writeValue ("FILT2", new_value->getValueInteger (), '!');
		} */
	if (old_value == msteps)
	{
		return writeValue ("GS", new_value->getValueInteger (), '!');
	}
	if (old_value == grat)
	{
		return writeValue ("GRAT", new_value->getValueInteger (), '!');
	}
	return Rts2DevSensorNewport::setValue (old_value, new_value);
}


int
Rts2DevSensorMS257::init ()
{
	int ret;

	ret = Rts2DevSensorNewport::init ();
	if (ret)
		return ret;

	ret = readRts2Value ("VER", msVer);
	if (ret)
		return -1;

	// sets correct units
	ret = writeValue ("UNITS", "nn");
	if (ret)
		return ret;

	// open shutter - init
	ret = writeValue ("SHUTTER", 0, '!');
	if (ret)
		return ret;

	return 0;
}


int
Rts2DevSensorMS257::info ()
{
	int ret;
	ret = readRts2Value ("PW", wavelenght);
	if (ret)
		return ret;
	ret = readRts2Value ("SLITA", slitA);
	if (ret)
		return ret;
	ret = readRts2Value ("SLITB", slitB);
	if (ret)
		return ret;
	ret = readRts2Value ("SLITC", slitC);
	if (ret)
		return ret;
	ret = readRts2Value ("BANDPASS", bandPass);
	if (ret)
		return ret;
	char *shttype;
	char **shttype_p = &shttype;
	ret = readValue ("SHTRTYPE", shttype_p);
	switch (**shttype_p)
	{
		case 'S':
			shutter->setValueInteger (0);
			break;
		case 'F':
			shutter->setValueInteger (1);
			break;
		case 'M':
			shutter->setValueInteger (2);
			break;
		default:
			logStream (MESSAGE_ERROR) << "Unknow shutter value: " << *shttype <<
				sendLog;
			return -1;
	}
	ret = readRts2ValueFilter ("FILT1", filter1);
	if (ret)
		return ret;
	/*  ret = readRts2Value ("FILT2", filter2);
	  if (ret)
		return ret;
	*/
	ret = readRts2Value ("PS", msteps);
	if (ret)
		return ret;
	usleep (100);
	ret = readRts2ValueFilter ("GRAT", grat);
	if (ret)
		return ret;
	return Rts2DevSensor::info ();
}


int
main (int argc, char **argv)
{
	Rts2DevSensorMS257 device = Rts2DevSensorMS257 (argc, argv);
	return device.run ();
}
