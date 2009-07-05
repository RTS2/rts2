/* 
 * Class for GPIB sensors.
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

#include "sensorgpib.h"

using namespace rts2sensord;


int
Gpib::processOption (int _opt)
{
	switch (_opt)
	{
		case 'm':
			connGpib->setMinor (atoi (optarg));
			break;
		case 'p':
			connGpib->setPad (atoi (optarg));
			break;
		default:
			return Sensor::processOption (_opt);
	}
	return 0;
}


int
Gpib::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;

	return connGpib->init ();
}


Gpib::Gpib (int argc, char **argv):Sensor (argc, argv)
{
	connGpib = new ConnGpib (this);

	addOption ('m', "minor", 1, "board number (default to 0)");
	addOption ('p', "pad", 1, "device number (counted from 0, not from 1)");
}


Gpib::~Gpib (void)
{
	delete connGpib;
}
