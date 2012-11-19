/* 
 * Variant of SCPI connection used by ThorLabs.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "serial.h"

namespace rts2core
{

typedef enum {LASER, FW} modelTypes;

/**
 * Connection for ThorLabs variant of SCPI. ThorLabs uses local echo
 * and > to delimit reply start, command structure match SCPI.
 */
class ConnThorLabs:public ConnSerial
{
	public:
		ConnThorLabs (const char *device_file, Block *master, int thorlabsType);

		int getValue (const char *name, rts2core::Value *value);
		int setValue (const char *name, rts2core::Value *value);

		int getInt (const char *name, int &value);
		int setInt (const char *name, int value);
	
	private:
		// connection type
		// 0 works for Laser, 1 for Filter wheels (which has local echo)
		int thorlabsType;
};

}
