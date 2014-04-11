/* 
 * Sitech connection.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

namespace rts2teld
{

/**
 * Sitech protocol connections. Used to control Sitech mounts.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnSitech: public rts2core::ConnSerial
{
	public:
		/**
		 * Construct new Sitech connection.
		 *
		 * @param devName device name
		 * @param master reference to master block
		 */
		ConnSitech (const char *devName, rts2core::Block *master);

		void siTechCommand (const char axis, const char *cmd);
};

}
