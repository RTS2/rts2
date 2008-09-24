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
Ford::zjisti_stav_portu ()
{
	char ta, tb;
	char ca = STAV_PORTU | PORT_A;

	if (domeConn->writePort (ca))
		return -1;
	if (domeConn->readPort (ta) != 1)
	  	return -1;
	if (domeConn->readPort (ca) != 1)
		return -1;
	
	char cb = STAV_PORTU | PORT_B;

	if (domeConn->writePort (cb))
		return -1;
	if (domeConn->readPort (tb) != 1)
	  	return -1;
	if (domeConn->readPort (cb) != 1)
	  	return -1;

	if (stav_portu[PORT_A] != ca || stav_portu[PORT_B] != cb)
	{
		logStream (MESSAGE_DEBUG) << "A 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) ta 
			<< " state 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) ca
			<< " B 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) tb
			<< " state 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) cb << sendLog;
	}

	stav_portu[PORT_A] = ca;
	stav_portu[PORT_B] = cb;

	return 0;
}


int
Ford::zapni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;
	int ret;

	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	c = ZAPIS_NA_PORT | c_port;

	if (domeConn->writePort (c))
		return -1;

	c = stav_portu[c_port] | pin;

	domeConn->writePort (c);
	return 0;
}


int
Ford::vypni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	c = ZAPIS_NA_PORT | c_port;

	if (domeConn->writePort (c))
	  	return -1;

	c = stav_portu[c_port] & (~pin);

	domeConn->writePort (c);
	return 0;
}


int
Ford::ZAP (int i)
{
	return zapni_pin (adresa[i].port, adresa[i].pin);
}


int
Ford::VYP (int i)
{
	return vypni_pin (adresa[i].port, adresa[i].pin);
}


int
Ford::switchOffPins (int pin1, int pin2)
{
	if (adresa[pin1].port != adresa[pin2].port)
	{
		logStream (MESSAGE_ERROR) << "pin address do not match! pin1: " 
			<< pin1 << " pin2: " << pin2 << sendLog;
		return -1;

	}
	return zapni_pin (adresa[pin1].port, adresa[pin1].pin | adresa [pin2].pin);
}


int
Ford::switchOffPins (int pin1, int pin2, int pin3)
{
	if (adresa[pin1].port != adresa[pin2].port
		|| adresa[pin1].port != adresa[pin3].port)
	{
		logStream (MESSAGE_ERROR) << "pin address do not match! pin1: " 
			<< pin1 << " pin2: " << pin2 << " pin3: " << pin3 << sendLog;
		return -1;

	}
	return zapni_pin (adresa[pin1].port, adresa[pin1].pin | adresa [pin2].pin | adresa[pin3].pin);
}


int
Ford::switchOffPins (int pin1, int pin2, int pin3, int pin4)
{
	if (adresa[pin1].port != adresa[pin2].port
		|| adresa[pin1].port != adresa[pin3].port
		|| adresa[pin1].port != adresa[pin4].port)
	{
		logStream (MESSAGE_ERROR) << "pin address do not match! pin1: " 
			<< pin1 << " pin2: " << pin2 << " pin3: " << pin3 
			<< pin4 << " pin4: " << pin4 << sendLog;
		return -1;

	}
	return zapni_pin (adresa[pin1].port, adresa[pin1].pin | adresa [pin2].pin | adresa[pin3].pin | adresa[pin4].port);
}


bool
Ford::getPortState (int c_port)
{
	return (stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}


int
Ford::isOn (int c_port)
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;
	return !(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
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

	domeConn = new Rts2ConnSerial (dome_file, this, BS9600, C8, NONE, 40);
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
