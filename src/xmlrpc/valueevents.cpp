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

#include "valueevents.h"
#include "message.h"

#include "../utils/timestamp.h"
#include "../utils/rts2block.h"
#include "../utils/rts2logstream.h"

#include <config.h>

using namespace rts2xmlrpc;

#ifndef HAVE_PGSQL

void ValueChangeCommand::run (Rts2Value *val, double validTime)
{
	std::cout << Timestamp (validTime) << " value: " << deviceName << " " << valueName << val->getDisplayValue () << std::endl;
}

#endif /* ! HAVE_PGSQL */
