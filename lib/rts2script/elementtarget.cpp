/*
 * Scripting support for target manipulations.
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#include "rts2script/elementtarget.h"
#include "rts2script/operands.h"

using namespace rts2script;

int ElementDisable::defnextCommand (__attribute__ ((unused)) rts2core::DevClient * client, __attribute__ ((unused)) rts2core::Command ** new_command, __attribute__ ((unused)) char new_device[DEVICE_NAME_SIZE])
{
	getTarget ()->setTargetEnabled (false);
	getTarget ()->save (true);
	return NEXT_COMMAND_NEXT;
}

int ElementTempDisable::defnextCommand (__attribute__ ((unused)) rts2core::DevClient * client, __attribute__ ((unused)) rts2core::Command ** new_command, __attribute__ ((unused)) char new_device[DEVICE_NAME_SIZE])
{
  	time_t now;
	time (&now);
	now += getDistTimeSec ();

	getTarget ()->setNextObservable (&now);
	getTarget ()->save (true);

	return NEXT_COMMAND_NEXT;
}

double ElementTempDisable::getDistTimeSec ()
{
	rts2operands::OperandsSet os;

	rts2operands::Operand *op = os.parseOperand (distime, rts2operands::MUL_TIME);
	double ret = op->getDouble ();
	delete op;

	return ret;
}

int ElementTarBoost::defnextCommand (__attribute__ ((unused)) rts2core::DevClient * client, __attribute__ ((unused)) rts2core::Command ** new_command, __attribute__ ((unused)) char new_device[DEVICE_NAME_SIZE])
{
	time_t now;
	time (&now);
	now += seconds;
	getTarget ()->setTargetBonus (bonus, &now);
	getTarget ()->save (true);
	return NEXT_COMMAND_NEXT;
}
