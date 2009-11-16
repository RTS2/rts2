/* 
 * Driver for Lakeshore 325 temperature controller.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

class Lakeshore;

/**
 * Values for a single channel.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TempChannel:public std::map < const char*, std::list <Rts2Value *> >
{
	public:
		TempChannel () {};
};

/**
 * Driver for Lakeshore 325 temperature controller.
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

		virtual int setValue (Rts2Value * oldValue, Rts2Value * newValue);

	private:
		TempChannel *temps[2];

		Rts2ValueBool *tunest;

		template < typename T> Rts2Value *tempValue (T *&val, const char *prefix, char chan, const char *_info, bool writeToFits)
		{
			std::ostringstream _os_n, _os_i;
			_os_n << prefix << "_" << chan;
			_os_i << chan << " " << _info;
			createValue (val, _os_n.str ().c_str (), _os_i.str ().c_str (), writeToFits);
			return val;
		}
};

}

using namespace rts2sensord;

Lakeshore::Lakeshore (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	for (int i = 0; i < 2; i++)
	{
		char chan = 'A' + i;
		temps[i] = new TempChannel ();
		Rts2ValueDouble *vd;
		Rts2ValueInteger *vi;
		(*(temps[i]))["CRDG"].push_back (tempValue (vd, "TEMP", chan, "channel temperature", true));
		(*(temps[i]))["INCRV"].push_back (tempValue (vi, "INCRV", chan, "channel input curve number", true));
		(*(temps[i]))["RDGST"].push_back (tempValue (vi, "STATUS", chan, "channel input reading status", true));
		(*(temps[i]))["SRDG"].push_back (tempValue (vd, "SENSOR", chan, "channel input sensor value", true));
	}

	createValue (tunest, "TUNEST", "if either Loop 1 or Loop 2 is actively tuning.", true);
}


Lakeshore::~Lakeshore (void)
{
	for (int i = 0; i < 2; i++)
	{
		delete temps[i];
		temps[i] = NULL;
	}
}

int Lakeshore::info ()
{
	try
	{
		for (int i = 0; i < 2; i++)
		{
			char chan = 'A' + i;
			for (std::map <const char*, std::list <Rts2Value*> >::iterator iter = temps[i]->begin (); iter != temps[i]->end (); iter++)
			{
				std::ostringstream _os;
				_os << iter->first << "? " << chan;

				char buf[100];
				gpibWriteRead (_os.str ().c_str (), buf, 100);
				std::vector <std::string> sc = SplitStr (std::string (buf), std::string (","));

				std::vector<std::string>::iterator iter_sc = sc.begin ();
				std::list <Rts2Value*>::iterator iter_v = iter->second.begin ();
				for (; iter_sc != sc.end () && iter_v != iter->second.end (); iter_sc++, iter_v++)
				{
					(*iter_v)->setValueCharArr (iter_sc->c_str ());
				}
			}
		}
		readValue ("TUNEST?", tunest);
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
	return Gpib::init ();
}

int Lakeshore::initValues ()
{
	Rts2ValueString *model = new Rts2ValueString ("model");
	readValue ("*IDN?", model);
	addConstValue (model);
	return Gpib::initValues ();
}

int Lakeshore::setValue (Rts2Value * oldValue, Rts2Value * newValue)
{
/*	try
	{
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -2;
	} */
	return Gpib::setValue (oldValue, newValue);
}

int main (int argc, char **argv)
{
	Lakeshore device (argc, argv);
	return device.run ();
}
