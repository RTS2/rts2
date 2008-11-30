/* 
 * System sensor, displaying free disk space and more.
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

#include "sensord.h"

#include <sys/vfs.h>

namespace rts2sensor
{

/**
 * This class is for sensor which displays informations about
 * system states.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class System:public Rts2DevSensor
{
	private:
		std::vector <std::string> paths;
	protected:
		virtual int info ();
	public:
		System (int in_argc, char **in_argv);

};

};

using namespace rts2sensor;

int
System::info ()
{
	for (std::vector <std::string>::iterator iter = paths.begin (); iter != paths.end (); iter++)
	{
		struct statfs sf;
		if (statfs ((*iter).c_str (), &sf))
		{
			logStream (MESSAGE_ERROR) << "Cannot get status for " << (*iter) << ". Error " << strerror (errno) << sendLog;
		}
		else
		{
			Rts2Value *val = getValue ((*iter).c_str ());
			if (val)
				((Rts2ValueLong *) val)->setValueLong (sf.f_bavail * sf.f_bsize);	
		}
	}
	return Rts2DevSensor::info ();
}

System::System (int argc, char **argv):Rts2DevSensor (argc, argv)
{
	paths.push_back (std::string ("/home"));

	Rts2ValueLong *val;
	createValue (val, "/home", "free disk on /home", false, RTS2_DT_BYTESIZE);
}

int
main (int argc, char **argv)
{
	System device = System (argc, argv);
	return device.run ();
}
