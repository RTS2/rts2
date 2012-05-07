/*
    Max Dome II Driver
    Copyright (C) 2009 Ferran Casarramona (ferran.casarramona@gmail.com)
    Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "cupola.h"
#include "connection/serial.h"

#define MAXDOME_TIMEOUT	5		/* FD timeout in seconds */

// Start byte
#define START       0x01
#define MAX_BUFFER  15

// Message Destination
#define TO_MAXDOME  0x00
#define TO_COMPUTER 0x80

// Commands available
#define ABORT_CMD   0x03		// Abort azimuth movement
#define HOME_CMD    0x04		// Move until 'home' position is detected
#define GOTO_CMD    0x05		// Go to azimuth position
#define SHUTTER_CMD 0x06		// Send a command to Shutter
#define STATUS_CMD  0x07		// Retrieve status
#define TICKS_CMD   0x09		// Set the number of tick per revolution of the dome
#define ACK_CMD     0x0A		// ACK (?)
#define SETPARK_CMD 0x0B		// Set park coordinates and if need to park before to operating shutter

// Shutter commands
#define OPEN_SHUTTER            0x01
#define OPEN_UPPER_ONLY_SHUTTER 0x02
#define CLOSE_SHUTTER           0x03
#define EXIT_SHUTTER            0x04  // Command send to shutter on program exit
#define ABORT_SHUTTER           0x07


/**
 * MaxDome II Sirius driver.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MaxDomeII:public Cupola
{
	public:
		MaxDomeII (int argc, char **argv);
		virtual ~MaxDomeII ();

		virtual double getSplitWidth (double alt);
	
	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();
	
	private:
		const char *devFile;
        	rts2core::ConnSerial *sconn;

		rts2core::ValueInteger *shutter;

		/**
		 * Calculate message checksum. Works on full buffer, assuming
		 * the starting 0x01 is not included in it.
		 */
		signed char checkSum (char *msg, size_t len);

		void sendMessage (uint8_t cmd, char *args, size_t len);

		/**
		 * @param buffer minimal size must be at least MAX_BUFFER
		 */
		void readReply (char *buffer);

		/**
		 *
		 */
		void exchangeMessage (uint8_t cmd, char *args, size_t len, char *buffer);
};

MaxDomeII::MaxDomeII (int argc, char **argv):Cupola (argc, argv)
{
	devFile = "/dev/ttyS0";
	sconn = NULL;

	createValue (shutter, "shutter", "shutter state", false);

	addOption ('f', NULL, 1, "serial port (defaults to /dev/ttyS0)");
}

MaxDomeII::~MaxDomeII ()
{
	delete sconn;
}

double MaxDomeII::getSplitWidth (double alt)
{
	return 10;
}

int MaxDomeII::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devFile = optarg;
			break;
		default:
			return Dome::processOption (opt);
	}
	return 0;
}

int MaxDomeII::initHardware ()
{
	sconn = new rts2core::ConnSerial (devFile, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, MAXDOME_TIMEOUT * 10);
	sconn->setDebug (getDebug ());
	sconn->init ();

	return 0;
}

int MaxDomeII::info ()
{
	char buffer[MAX_BUFFER];
	exchangeMessage (STATUS_CMD, NULL, 0, buffer);

	setCurrentAz (((uint16_t) buffer[4]) << 8 | buffer[5]); 
	shutter->setValueInteger (buffer[2]);

	return Cupola::info ();
}

int MaxDomeII::startOpen ()
{
	char buffer[MAX_BUFFER];
	buffer[0] = OPEN_SHUTTER;

	exchangeMessage (SHUTTER_CMD, buffer, 1, buffer);
	return 0;
}

long MaxDomeII::isOpened ()
{
	if (info ())
		return -1;
	return shutter->getValueInteger () == 1 ? -2 : USEC_SEC;
}

int MaxDomeII::endOpen ()
{
	return 0;
}

int MaxDomeII::startClose ()
{
	char buffer[MAX_BUFFER];
	buffer[0] = CLOSE_SHUTTER;

	exchangeMessage (SHUTTER_CMD, buffer, 1, buffer);
	return 0;
}

long MaxDomeII::isClosed ()
{
	if (info ())
		return -1;
	return shutter->getValueInteger () == 0 ? -2 : USEC_SEC;
}

int MaxDomeII::endClose ()
{
	return 0;
}

signed char MaxDomeII::checkSum (char *msg, size_t len)
{
	char nChecksum = 0;

	for (size_t i = 0; i < len; i++)
		nChecksum -= msg[i];
	
	return nChecksum;
}

void MaxDomeII::sendMessage (uint8_t cmd, char *args, size_t len)
{
	sconn->writePort (START);
	sconn->writePort (len + 2);
	sconn->writePort (cmd);
	if (args != NULL)
	{
		sconn->writePort (args, len);
		sconn->writePort (checkSum (args, len) - cmd - (len + 2));
	}
	else
	{
		// checksum calculation in-place
		sconn->writePort (0 - cmd - (len + 2));
	}
}

void MaxDomeII::readReply (char *buffer)
{
	char c;
	while (true)
	{
		if (sconn->readPort (c) < 0)
			throw rts2core::Error ("cannot read start character from serial port");
		if (c == START)
			break;
	}

	// read length
	if (sconn->readPort (c) < 0)
		throw rts2core::Error ("cannot read length of the message");
	if (c < 0x02 || c > 0x0E)
		throw rts2core::Error ("invalid length of the packet");

	buffer[0] = c;
	
	if (sconn->readPort (buffer + 1, c))
		throw rts2core::Error ("cannot read full packet");

	if (checkSum (buffer, c + 1) != 0)
		throw rts2core::Error ("invalid checksum");
}

void MaxDomeII::exchangeMessage (uint8_t cmd, char *args, size_t len, char *buffer)
{
	sendMessage (cmd, args, len);
	readReply (buffer);
	if (buffer[1] != (cmd | TO_COMPUTER))
		throw rts2core::Error ("invalid reply - invalid command byte");
}

int main (int argc, char **argv)
{
	MaxDomeII device (argc, argv);
	return device.run ();
}
