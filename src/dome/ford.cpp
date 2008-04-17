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

#define CTENI_PORTU 0
#define ZAPIS_NA_PORT 4
#define STAV_PORTU 0

#include <iomanip>

struct typ_a
{
	unsigned char port;
	unsigned char pin;
} adresa[] =
{
	{PORT_B, 1},
	{PORT_B, 2},
	{PORT_B, 4},
	{PORT_B, 8},
	{PORT_B, 16},
	{PORT_B, 32},
	{PORT_B, 64},
	{PORT_B, 128},
	{PORT_A, 1},
	{PORT_A, 2},
	{PORT_A, 4},
	{PORT_A, 8},
	{PORT_A, 16},
	{PORT_A, 32},
	{PORT_A, 64},
	{PORT_A, 128}
};

int
Rts2DomeFord::zjisti_stav_portu ()
{
	char ta, tb;
	char c = STAV_PORTU | PORT_A;

	if (domeConn->writePort (c))
		return -1;
	if (domeConn->readPort (ta) != 1)
	  	return -1;
	if (domeConn->readPort (c) != 1)
		return -1;
	stav_portu[PORT_A] = c;
	
	c = STAV_PORTU | PORT_B;

	if (domeConn->writePort (c))
		return -1;
	if (domeConn->readPort (tb) != 1)
	  	return -1;
	if (domeConn->readPort (c) != 1)
	  	return -1;
	stav_portu[PORT_B] = c;

	logStream (MESSAGE_DEBUG) << "A 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) ta 
		<< " state 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) stav_portu[PORT_A]
		<< " B 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) tb
		<< " state 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int)stav_portu[PORT_B] << sendLog;
	
	return 0;
}


void
Rts2DomeFord::zapni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;

	zjisti_stav_portu ();

	c = ZAPIS_NA_PORT | c_port;

	if (domeConn->writePort (c))
		return;

	c = stav_portu[c_port] | pin;

	domeConn->writePort (c);
}


void
Rts2DomeFord::vypni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;
	zjisti_stav_portu ();

	c = ZAPIS_NA_PORT | c_port;

	if (domeConn->writePort (c))
	  	return;

	c = stav_portu[c_port] & (~pin);

	domeConn->writePort (c);
}


void
Rts2DomeFord::ZAP (int i)
{
	zapni_pin (adresa[i].port, adresa[i].pin);
}


void
Rts2DomeFord::VYP (int i)
{
	vypni_pin (adresa[i].port, adresa[i].pin);
}


bool
Rts2DomeFord::getPortState (int c_port)
{
	return (stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}


int
Rts2DomeFord::isOn (int c_port)
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;
	return !(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}


int
Rts2DomeFord::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dome_file = optarg;
			break;
		default:
			Rts2DevDome::processOption (in_opt);
	}
	return 0;
}


int
Rts2DomeFord::init ()
{
	int ret;
	ret = Rts2DevDome::init ();
	if (ret)
		return ret;

	domeConn = new Rts2ConnSerial (dome_file, this, BS9600, C8, NONE, 40);
	ret = domeConn->init ();
	if (ret)
		return ret;

	domeConn->flushPortIO ();
	return 0;
}


Rts2DomeFord::Rts2DomeFord (int in_argc, char **in_argv)
:Rts2DevDome (in_argc, in_argv)
{
	dome_file = "/dev/ttyS0";
	domeConn = NULL;
	addOption ('f', NULL, 1, "/dev file for dome serial port (default to /dev/ttyS0)");
}


Rts2DomeFord::~Rts2DomeFord (void)
{
	delete domeConn;
}
