/* 
 * Driver for Ford boards.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define BAUDRATE B9600
#define CTENI_PORTU 0
#define ZAPIS_NA_PORT 4
#define STAV_PORTU 0

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
	unsigned char ta, tb, c = STAV_PORTU | PORT_A;
	int ret;
	write (dome_port, &c, 1);
	if (read (dome_port, &ta, 1) < 1)
		logStream (MESSAGE_ERROR) << "read error 0" << sendLog;
	read (dome_port, &stav_portu[PORT_A], 1);
	c = STAV_PORTU | PORT_B;
	write (dome_port, &c, 1);
	if (read (dome_port, &tb, 1) < 1)
		logStream (MESSAGE_ERROR) << "read error 1" << sendLog;
	ret = read (dome_port, &stav_portu[PORT_B], 1);
	logStream (MESSAGE_DEBUG) << "A stav:" << ta << " state:" <<
		stav_portu[PORT_A] << " B stav: " << tb << " state: " <<
		stav_portu[PORT_B] << sendLog;
	if (ret < 1)
		return -1;
	return 0;
}


void
Rts2DomeFord::zapni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;
	zjisti_stav_portu ();
	c = ZAPIS_NA_PORT | c_port;
	logStream (MESSAGE_DEBUG) << "port:" << c_port << " pin:" << pin <<
		" write:" << c << sendLog;
	write (dome_port, &c, 1);
	c = stav_portu[c_port] | pin;
	logStream (MESSAGE_DEBUG) << "zapni_pin: " << c << sendLog;
	write (dome_port, &c, 1);
}


void
Rts2DomeFord::vypni_pin (unsigned char c_port, unsigned char pin)
{
	unsigned char c;
	zjisti_stav_portu ();
	c = ZAPIS_NA_PORT | c_port;
	logStream (MESSAGE_DEBUG) << "port:" << c_port << " pin:" << pin <<
		" write:" << c << sendLog;
	write (dome_port, &c, 1);
	c = stav_portu[c_port] & (~pin);
	logStream (MESSAGE_DEBUG) << c << sendLog;
	write (dome_port, &c, 1);
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
	struct termios oldtio, newtio;

	int ret;
	ret = Rts2DevDome::init ();
	if (ret)
		return ret;
	dome_port = open (dome_file, O_RDWR | O_NOCTTY);

	if (dome_port == -1)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init open " <<
			strerror (errno) << sendLog;
		return -1;
	}

	ret = tcgetattr (dome_port, &oldtio);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init tcgetattr " <<
			strerror (errno) << sendLog;
		return -1;
	}

	newtio = oldtio;

	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 1;

	tcflush (dome_port, TCIOFLUSH);
	ret = tcsetattr (dome_port, TCSANOW, &newtio);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2DomeFord::init tcsetattr " <<
			strerror (errno) << sendLog;
		return -1;
	}

	return 0;
}


Rts2DomeFord::Rts2DomeFord (int in_argc, char **in_argv)
:Rts2DevDome (in_argc, in_argv)
{
	dome_file = "/dev/ttyS0";
	dome_port = 0;
	addOption ('f', NULL, 1, "/dev file for dome serial port (default to /dev/ttyS0)");
}


Rts2DomeFord::~Rts2DomeFord (void)
{
	if (dome_port)
		close (dome_port);
}
