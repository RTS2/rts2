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
#include "rts2-config.h"

#ifdef RTS2_HAVE_GPIB_IB_H
#include "connection/conngpiblinux.h"
#endif

#include "connection/conngpibenet.h"
#include "connection/conngpibprologix.h"
#include "connection/conngpibserial.h"

using namespace rts2sensord;

#define OPT_PROLOGIX        OPT_LOCAL + 787

// serial port options
#define OPT_SERIAL_BAUD     OPT_LOCAL + 788
#define OPT_SERIAL_PARITY   OPT_LOCAL + 789
#define OPT_SERIAL_CSIZE    OPT_LOCAL + 790
#define OPT_SERIAL_SEP      OPT_LOCAL + 791


Gpib::Gpib (int argc, char **argv):Sensor (argc, argv)
{
	minor = 0;
	pad = -1;
	enet_addr = NULL;
	tcp_addr = NULL;
	prologix = NULL;
	serial_port = NULL;
	
	idn = NULL;

	serial_baud = rts2core::BS9600;
	serial_csize = rts2core::C8;
	serial_parity = rts2core::NONE;
	serial_sep = "\n";

	connGpib = NULL;
	debug = false;
	replyWithValueName = false;
	boolOnOff = true;

	addOption ('m', NULL, 1, "board number (default to 0)");
	addOption (OPT_SERIAL_SEP, "serial-sep", 1, "serial separator (default to LF)");
	addOption (OPT_SERIAL_PARITY, "serial-parity", 1, "serial parity (default to NONE)");
	addOption (OPT_SERIAL_CSIZE, "serial-csize", 1, "serial bits per character (default to 7)");
	addOption (OPT_SERIAL_BAUD, "serial-baud", 1, "serial baud speed (default to 9600)");
	addOption ('s', NULL, 1, "RS-232 (serial) for GPIB commands send over serial port");
	addOption (OPT_PROLOGIX, "prologix", 1, "Prologix GPIB-USB serial port");
	addOption ('p', NULL, 1, "device number (counted from 0, not from 1)");
	addOption ('n', NULL, 1, "network adress (and port) of NI GPIB-ENET interface");
	addOption ('t', NULL, 1, "network adress (and port) of raw TCP/IP socket");
	addOption ('v', NULL, 0, "verbose debugging");
}

Gpib::~Gpib (void)
{
	delete connGpib;
	delete enet_addr;
	delete tcp_addr;
}

void Gpib::writeValue (const char *name, rts2core::Value *value)
{
	std::ostringstream _os;
	_os << name << " ";
	switch (value->getValueExtType ())
	{
		case RTS2_VALUE_MMAX:
			switch (value->getValueBaseType ())
			{
				case RTS2_VALUE_DOUBLE:
					_os << value->getValueDouble ();
					break;
				case RTS2_VALUE_INTEGER:
					_os << value->getValueInteger ();
					break;
			}
			break;
		default:
			switch (value->getValueType ())
			{
				case RTS2_VALUE_BOOL:
					if (boolOnOff)
						_os << (((rts2core::ValueBool *) value)->getValueBool () ? "ON" : "OFF");
					else
						_os << (((rts2core::ValueBool *) value)->getValueBool () ? "1" : "0");
					break;
				default:
					_os << value->getDisplayValue ();
			}
	}
	gpibWrite (_os.str ().c_str ());
}

int Gpib::processOption (int _opt)
{
	switch (_opt)
	{
		case 'm':
			minor = atoi (optarg);
			break;
		case 'p':
			pad = atoi (optarg);
			break;
		case 'n':
			enet_addr = new HostString (optarg, "5000");
			break;
		case 't':
			tcp_addr = new HostString (optarg, "9221");
			break;
		case OPT_PROLOGIX:
			prologix = optarg;
			break;
		case 's':
			serial_port = optarg;
			break;
		case OPT_SERIAL_BAUD:
			if (!strcmp (optarg, "1200"))
				serial_baud = rts2core::BS1200;
			else if (!strcmp (optarg, "9600"))
				serial_baud = rts2core::BS9600;
			else if (!strcmp (optarg, "57600"))
				serial_baud = rts2core::BS57600;
			else
			{
				std::cerr << "invalid baud speed " << optarg << ", must be 1200, 9600 or 57600" << std::endl;
				return -1;
			}
			break;
		case OPT_SERIAL_CSIZE:
			if (!strcmp (optarg, "7"))
				serial_csize = rts2core::C7;
			else if (!strcmp (optarg, "8"))
				serial_csize = rts2core::C8;
			else
			{
				std::cerr << "invalid character length " << optarg << ", must be 7 or 8" << std::endl;
				return -1;
			}
			break;
		case OPT_SERIAL_PARITY:
			if (!strcmp (optarg, "NONE"))
				serial_parity = rts2core::NONE;
			else if (!strcmp (optarg, "ODD"))
				serial_parity = rts2core::ODD;
			else if (!strcmp (optarg, "EVEN"))
				serial_parity = rts2core::EVEN;
			else
			{
				std::cerr << "invalid serial parity " << optarg << ", must be NONE, ODD or EVEN" << std::endl;
				return -1;
			}
			break;
		case OPT_SERIAL_SEP:
			if (!strcmp (optarg, "LF"))
				serial_sep = "\n";
			else if (!strcmp (optarg, "CRLF"))
				serial_sep = "\r\n";
			else
			{
				std::cerr << "invalid serial separator " << optarg << ", must be LF or CRLF" << std::endl;
				return -1;
			}
			break;
		case 'v':
			debug = true;
			break;
		default:
			return Sensor::processOption (_opt);
	}
	return 0;
}

int Gpib::initValues ()
{
	idn = new rts2core::ValueString ("IDN", "identification string", true);
	readValue ("*IDN?", idn);
	addConstValue (idn);
	return Sensor::initValues ();
}

int Gpib::initHardware ()
{
	// create connGpin
	if (enet_addr != NULL)
	{
		if (pad < 0)
		{
			std::cerr << "unknown pad number" << std::endl;
			return -1;
		}
		connGpib = new rts2core::ConnGpibEnet (this, enet_addr->getHostname (), enet_addr->getPort (), pad);
	}
	if (tcp_addr != NULL)
	{

	}
	// prologix USB
	else if (prologix != NULL)
	{
		connGpib = new rts2core::ConnGpibPrologix (this, prologix, pad);
	}
	else if (serial_port != NULL)
	{
		connGpib = new rts2core::ConnGpibSerial (this, serial_port, serial_baud, serial_csize, serial_parity, serial_sep);
	}
#ifdef RTS2_HAVE_GPIB_IB_H
	else if (pad >= 0)
	{
		connGpib = new rts2core::ConnGpibLinux (minor, pad);
	}
#endif
	else
	{
		std::cerr << "Device connection was not specified, exiting" << std::endl;
	}

	try
	{
		connGpib->setReplyWithValueName (replyWithValueName);
		connGpib->setDebug (debug);
		connGpib->initGpib ();
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

void Gpib::setReplyWithValueName (bool on)
{
	replyWithValueName = on;
	if (connGpib != NULL)
		connGpib->setReplyWithValueName (on);
}
