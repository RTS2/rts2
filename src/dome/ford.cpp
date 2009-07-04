/* 
 * Driver for Ford boards.
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

#include "ford.h"

using namespace rts2dome;

#include <iomanip>

int
Ford::zjisti_stav_portu ()
{
	return domeConn->zjisti_stav_portu ();
}


int
Ford::ZAP (int i)
{
	return domeConn->ZAP (i);
}


int
Ford::VYP (int i)
{
	return domeConn->VYP (i);
}


int
Ford::switchOffPins (int pin1, int pin2)
{
	return domeConn->switchOffPins (pin1, pin2);
}


int
Ford::switchOffPins (int pin1, int pin2, int pin3)
{
	return domeConn->switchOffPins (pin1, pin2, pin3);
}


int
Ford::switchOffPins (int pin1, int pin2, int pin3, int pin4)
{
	return domeConn->switchOffPins (pin1, pin2, pin3, pin4);
}


bool
Ford::getPortState (int c_port)
{
	return domeConn->getPortState (c_port);
}


int
Ford::isOn (int c_port)
{
	return domeConn->isOn (c_port);
}


int
Ford::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dome_file = optarg;
			break;
		default:
			Dome::processOption (in_opt);
	}
	return 0;
}


int
Ford::init ()
{
	int ret;
	ret = Dome::init ();
	if (ret)
		return ret;

	domeConn = new FordConn (dome_file, this, BS9600, C8, NONE, 40);
	ret = domeConn->init ();
	if (ret)
		return ret;

	domeConn->flushPortIO ();
	return 0;
}


Ford::Ford (int argc, char **argv)
:Dome (argc, argv)
{
	dome_file = "/dev/ttyS0";
	domeConn = NULL;
	addOption ('f', NULL, 1, "/dev file for dome serial port (default to /dev/ttyS0)");
}


Ford::~Ford (void)
{
	delete domeConn;
}
