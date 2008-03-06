/* 
 * Class for Newport serial I/O.
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

#ifndef __RTS2_SENSOR_NEWPORT__
#define __RTS2_SENSOR_NEWPORT__

#include "sensord.h"

class Rts2DevSensorNewport:public Rts2DevSensor
{
	private:
		int dev_port;

	protected:
		void resetDevice ();

		int writePort (const char *str);

		int readPort (char **rstr, const char *cmd);

		// specialized readPort functions..
		int readPort (int &ret, const char *cmd);
		int readPort (double &ret, const char *cmd);

		template < typename T > int writeValue (const char *valueName, T val,
			char qStr = '=');
		template < typename T > int readValue (const char *valueName, T & val);

		int readRts2Value (const char *valueName, Rts2Value * val);

		char *dev;

		virtual int processOption (int in_opt);
		virtual int init ();
	public:
		Rts2DevSensorNewport (int in_argc, char **in_argv);
		virtual ~ Rts2DevSensorNewport (void);
};

template < typename T > int
Rts2DevSensorNewport::writeValue (const char *valueName, T val, char qStr)
{
	int ret;
	char *rstr;
	std::ostringstream _os;
	_os << qStr << valueName << ' ' << val;
	ret = writePort (_os.str ().c_str ());
	if (ret)
		return ret;
	ret = readPort (&rstr, _os.str ().c_str ());
	return ret;
}


template < typename T > int
Rts2DevSensorNewport::readValue (const char *valueName, T & val)
{
	int ret;
	char buf[strlen (valueName + 1)];
	*buf = '?';
	strcpy (buf + 1, valueName);
	ret = writePort (buf);
	if (ret)
		return ret;
	ret = readPort (val, buf);
	return ret;
}
#endif							 /* !__RTS2_SENSOR_NEWPORT__ */
