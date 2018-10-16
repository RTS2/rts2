/* 
 * Connection to Ford boards.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#include "connection/ford.h"

using namespace rts2core;

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
FordConn::zjisti_stav_portu ()
{
	char ta, tb;
	char ca = STAV_PORTU | PORT_A;

	if (writePort (ca))
		return -1;
	if (readPort (ta) != 1)
	  	return -1;
	if (readPort (ca) != 1)
		return -1;
	
	char cb = STAV_PORTU | PORT_B;

	if (writePort (cb))
		return -1;
	if (readPort (tb) != 1)
	  	return -1;
	if (readPort (cb) != 1)
	  	return -1;

	if (stav_portu[PORT_A] != ca || stav_portu[PORT_B] != cb)
	{
		int p_ca = ca;
		int p_cb = cb;
		int p_ta = ta;
		int p_tb = tb;
		logStream (MESSAGE_DEBUG) << "A 0x" << std::hex << std::setw (2) << std::setfill ('0') << p_ta 
			<< " state 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) p_ca
			<< " B 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) p_tb
			<< " state 0x" << std::hex << std::setw (2) << std::setfill ('0') << (int) p_cb << sendLog;
	}

	stav_portu[PORT_A] = ca;
	stav_portu[PORT_B] = cb;

	return 0;
}


int
FordConn::zapni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;
	int ret;

	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	c = ZAPIS_NA_PORT | c_port;

	if (writePort (c))
		return -1;

	c = stav_portu[c_port] | pin;

	writePort (c);
	return 0;
}


int
FordConn::vypni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	c = ZAPIS_NA_PORT | c_port;

	if (writePort (c))
	  	return -1;

	c = stav_portu[c_port] & (~pin);

	writePort (c);
	return 0;
}


int
FordConn::ZAP (int i)
{
	return zapni_pin (adresa[i].port, adresa[i].pin);
}


int
FordConn::VYP (int i)
{
	return vypni_pin (adresa[i].port, adresa[i].pin);
}


int
FordConn::switchOffPins (int pin1, int pin2)
{
	if (adresa[pin1].port != adresa[pin2].port)
	{
		logStream (MESSAGE_ERROR) << "pin address do not match! pin1: " 
			<< pin1 << " pin2: " << pin2 << sendLog;
		return -1;

	}
	return vypni_pin (adresa[pin1].port, adresa[pin1].pin | adresa [pin2].pin);
}


int
FordConn::switchOffPins (int pin1, int pin2, int pin3)
{
	if (adresa[pin1].port != adresa[pin2].port
		|| adresa[pin1].port != adresa[pin3].port)
	{
		logStream (MESSAGE_ERROR) << "pin address do not match! pin1: " 
			<< pin1 << " pin2: " << pin2 << " pin3: " << pin3 << sendLog;
		return -1;

	}
	return vypni_pin (adresa[pin1].port, adresa[pin1].pin | adresa [pin2].pin | adresa[pin3].pin);
}


int
FordConn::switchOffPins (int pin1, int pin2, int pin3, int pin4)
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
	return vypni_pin (adresa[pin1].port, adresa[pin1].pin | adresa [pin2].pin | adresa[pin3].pin | adresa[pin4].port);
}


bool
FordConn::getPortState (int c_port)
{
	return (stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}


int
FordConn::isOn (int c_port)
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;
	return !(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}

FordConn::FordConn (const char *_devName, rts2core::Block * _master, bSpeedT _baudSpeed, cSizeT _cSize, parityT _parity, __attribute__ ((unused)) int _vTime):ConnSerial (_devName, _master, _baudSpeed, _cSize, _parity)
{

}


FordConn::~FordConn (void)
{
}
