/*
 * Script element for hex movement.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "rts2script/script.h"
#include "elementhex.h"

using namespace rts2script;

Rts2Path::~Rts2Path (void)
{
	for (top = begin (); top != end (); top++)
	{
		delete *top;
	}
	clear ();
}

void Rts2Path::addRaDec (double in_ra, double in_dec)
{
	struct ln_equ_posn *newPos = new struct ln_equ_posn;
	newPos->ra = in_ra;
	newPos->dec = in_dec;
	push_back (newPos);
}

double ElementHex::getRa ()
{
	return path.getRa () * ra_size;
}

double ElementHex::getDec ()
{
	return path.getDec () * dec_size;
}

bool ElementHex::endLoop ()
{
	return !path.haveNext ();
}

bool ElementHex::getNextLoop ()
{
	if (path.getNext ())
	{
		changeEl->setChangeRaDec (getRa (), getDec ());
		return false;
	}
	afterBlockEnd ();
	return true;
}

void ElementHex::constructPath ()
{
	// construct path
	#define SQRT3 0.866
	path.addRaDec (-1, 0);
	path.addRaDec (0.5, SQRT3);
	path.addRaDec (1, 0);
	path.addRaDec (0.5, -SQRT3);
	path.addRaDec (-0.5, -SQRT3);
	path.addRaDec (-1, 0);
	path.addRaDec (0.5, SQRT3);
	#undef SQRT3
	path.endPath ();
}

void ElementHex::afterBlockEnd ()
{
	ElementBlock::afterBlockEnd ();
	path.rewindPath ();
	bool en = true;
	script->getMaster ()->postEvent (new rts2core::Event (EVENT_QUICK_ENABLE, (void *) &en));
}

ElementHex::ElementHex (Script * in_script, char new_device[DEVICE_NAME_SIZE], double in_ra_size, double in_dec_size):ElementBlock (in_script)
{
	deviceName = new char[strlen (new_device) + 1];
	strcpy (deviceName, new_device);
	ra_size = in_ra_size;
	dec_size = in_dec_size;
	changeEl = NULL;
}

ElementHex::~ElementHex (void)
{
	changeEl = NULL;
}

void ElementHex::beforeExecuting ()
{
	if (!path.haveNext ())
		constructPath ();

	if (path.haveNext ())
	{
		changeEl = new ElementChange (script, deviceName, getRa (), getDec ());
		addElement (changeEl);
	}

	bool en = false;
	if (script->getMaster ())
		script->getMaster ()->postEvent (new rts2core::Event (EVENT_QUICK_ENABLE, (void *) &en));
}

void ElementFxF::constructPath ()
{
	path.addRaDec (-2, -2);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (0, 1);
	path.addRaDec (-1, 0);
	path.addRaDec (-1, 0);
	path.addRaDec (-1, 0);
	path.addRaDec (-1, 0);
	path.addRaDec (0, 1);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (0, 1);
	path.addRaDec (-1, 0);
	path.addRaDec (-1, 0);
	path.addRaDec (-1, 0);
	path.addRaDec (-1, 0);
	path.addRaDec (0, 1);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (1, 0);
	path.addRaDec (-2, -2);
	path.endPath ();
}

ElementFxF::ElementFxF (Script * in_script, char new_device[DEVICE_NAME_SIZE], double in_ra_size, double in_dec_size):ElementHex (in_script, new_device, in_ra_size, in_dec_size)
{
}

ElementFxF::~ElementFxF (void)
{
}
