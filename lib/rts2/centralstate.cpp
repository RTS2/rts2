/* 
 * Central server state.
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

#include <sstream>
#include "status.h"

#include "centralstate.h"

using namespace rts2core;

const char * CentralState::getStringShort (int _state)
{
	switch (_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_DAY:
			return "day";
			break;
		case SERVERD_EVENING:
			return "evening";
		case SERVERD_DUSK:
			return "dusk";
		case SERVERD_NIGHT:
			return "night";
		case SERVERD_DAWN:
			return "dawn";
		case SERVERD_MORNING:
			return "morning";
	}
	return "unknow";
}

std::string CentralState::getString (int _state)
{
	std::ostringstream os;
	// check for weather
	if ((_state & WEATHER_MASK) == BAD_WEATHER)
	{
		os << "bad weather | ";
	}
	if ((_state & STOP_MASK) == STOP_EVERYTHING)
	{
		os << "stop | ";
	}
	if ((_state & SERVERD_ONOFF_MASK) == SERVERD_HARD_OFF)
	{
		os << "HARD OFF ";
        }
	else if ((_state & SERVERD_ONOFF_MASK) == SERVERD_SOFT_OFF)
	{
		os << "SOFT OFF ";
	}
	else if ((_state & SERVERD_ONOFF_MASK) == SERVERD_STANDBY)
	{
		os << "standby ";
	}
	else
	{
		os << "ON ";
	}
	os << getStringShort (_state);
	// check for blocking
	if (_state & BOP_EXPOSURE)
		os << ", block exposure";
	if (_state & BOP_READOUT)
		os << ", block readout";
	if (_state & BOP_TEL_MOVE)
		os << ", block telescope move";
	return os.str ();
}
