/* 
 * Script executor interface.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SCRIPTINTERFACE__
#define __RTS2_SCRIPTINTERFACE__

#include "nan.h"

#include <string>
#include <iostream>
#include <fstream>
#include <math.h>
#include <libnova/libnova.h>

namespace rts2script
{

/**
 * Holds script for given device.
 */
class ScriptForDevice
{
	public:
		ScriptForDevice (std::string in_deviceName)
		{
			deviceName = in_deviceName;
		}

		ScriptForDevice (std::string in_deviceName, std::string in_script)
		{
			deviceName = in_deviceName;
			setScript (in_script);
		}

		virtual ~ ScriptForDevice (void)
		{
		}

		bool isDevice (std::string in_deviceName)
		{
			return deviceName == in_deviceName;
		}

		virtual int getScript (std::string & buf)
		{
			buf = script;
			return 0;
		}

	protected:
		void setDeviceName (std::string in_deviceName)
		{
			deviceName = in_deviceName;
		}

		/**
		 * Sets script string.
		 */
		void setScript (std::string in_script)
		{
			script = in_script;
		}

	private:
		std::string deviceName;
		std::string script;
};

/**
 * For cases when script is in file.
 */
class ScriptForDeviceStream:public ScriptForDevice
{
	public:
		ScriptForDeviceStream (std::string in_deviceName, std::istream *in_is):ScriptForDevice (in_deviceName)
		{
			setDeviceName (in_deviceName);
			is = in_is;
		}

		virtual ~ ScriptForDeviceStream (void)
		{
			delete is;
		}

		virtual int getScript (std::string & buf);

	private:
		std::istream *is;
};

class ScriptInterface
{
	public:
		ScriptInterface ()
		{
		}

		virtual ~ScriptInterface (void)
		{
		}

		/**
		 * Return script for next target.
		 *
		 * @param in_deviceName   Name of device for script.
		 * @param buf             Buffer for device script.
		 *
		 * @return NULL when script for given device cannot be found, otherwise device script.
		 */
		virtual int findScript (std::string in_deviceName, std::string & buf) = 0;

		/**
		 * Return position of next target.
		 *
		 * @param ln_equ_posn  Position of next target.
		 * @param JD           Julian data for which the position should be calculated.
		 */
		virtual void getPosition (struct ln_equ_posn *pos, __attribute__ ((unused))double JD) { pos->ra = pos->dec = NAN; }
};

}
#endif							 // !__RTS2_SCRIPTINTERFACE__
