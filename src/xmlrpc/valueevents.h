/* 
 * Value changes triggering infrastructure. 
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

#ifndef __RTS2__VALUEEVENTS__
#define __RTS2__VALUEEVENTS__

#include "../utils/rts2value.h"

#include <string>
#include <list>

namespace rts2xmlrpc
{

/**
 * Class triggered on value change.
 */
class ValueChangeCommand
{
	private:
		std::string deviceName;
		std::string valueName;

		int dbValueId;

	public:
		ValueChangeCommand (std::string _deviceName, std::string _valueName)
		{
			deviceName = _deviceName;
			valueName = _valueName;
			dbValueId = -1;
		}

		bool isForValue (std::string _deviceName, std::string _valueName)
		{
			return deviceName == _deviceName && valueName == _valueName;
		}

		/**
		 * Triggered when value is changed.
		 */
		void run (Rts2Value *val, double validTime);
};

/**
 * Holds list of ValueChangeCommands
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueCommands:public std::list <ValueChangeCommand>
{
	public:
		ValueCommands ()
		{
		}

		~ValueCommands ()
		{
		}
};

}

#endif /* !__RTS2__VALUEEVENTS__ */
