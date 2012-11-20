/* 
 * Generic serial port connection.
 * Copyright (C) 2008-2009 Petr Kubanek <petr@kubanek.net>
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

#include <fcntl.h>
#include <sys/ioctl.h>

#include "connection/serial.h"

#include "block.h"
#include <iomanip>

using namespace rts2core;

int ConnSerial::setAttr ()
{
	if (tcsetattr (sock, TCSANOW, &s_termios) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot set device parameters" << sendLog;
		return -1;
	}
	return 0;
}

void ConnSerial::flushError ()
{
	if (flushSleepTime >= 0)
	{
		sleep (flushSleepTime);
		flushPortIO ();
	}
}

ConnSerial::ConnSerial (const char *_devName, rts2core::Block * _master, bSpeedT _baudSpeed, cSizeT _cSize, parityT _parity, int _vTime, int _flushSleepTime):ConnNoSend (_master)
{
	sock = open (_devName, O_RDWR | O_NOCTTY | O_NDELAY);

	if (sock < 0)
		logStream (MESSAGE_ERROR) << "cannot open serial port:" << _devName << sendLog;

	// some defaults
	baudSpeed = _baudSpeed;

	cSize = _cSize;
	parity = _parity;

	vMin = 0;
	vTime = _vTime;

	flushSleepTime = _flushSleepTime;

	debugPortComm = false;
	logTrafficAsHex = false;
}

const char * ConnSerial::getBaudSpeed ()
{
	switch (baudSpeed)
	{
		case BS1200:
			return "1200";
		case BS2400:
			return "2400";
		case BS4800:
			return "4800";
		case BS9600:
			return "9600";
		case BS19200:
			return "19200";
		case BS57600:
			return "57600";
		case BS115200:
			return "115200";
	}
	return NULL;
}

int ConnSerial::init ()
{
	if (sock < 0)
		return -1;
	
	// set blocking mode
	fcntl (sock, F_SETFL, 0);

	speed_t b_speed;
	int ret;

	ret = tcgetattr (sock, &s_termios);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot get serial port attributes:" << strerror (errno) << sendLog;
		return -1;
	}

	switch (baudSpeed)
	{
		case BS1200:
			b_speed = B1200;
			break;
		case BS2400:
			b_speed = B2400;
			break;
		case BS4800:
			b_speed = B4800;
			break;
		case BS9600:
			b_speed = B9600;
			break;
		case BS19200:
			b_speed = B19200;
			break;
		case BS57600:
			b_speed = B57600;
			break;
		case BS115200:
			b_speed = B115200;
			break;
	}

	ret = cfsetospeed (&s_termios, b_speed);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot set output speed to " << getBaudSpeed () << sendLog;
		return -1;
	}

	ret = cfsetispeed (&s_termios, b_speed);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot set input speed to " << getBaudSpeed () << sendLog;
		return -1;
	}

	s_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
	s_termios.c_oflag = 0;
	// clear CSIZE and parity from cflags..
	s_termios.c_cflag = s_termios.c_cflag & ~(CSIZE | PARENB | PARODD | CSTOPB);
	switch (cSize)
	{
		case C7:
			s_termios.c_cflag |= CS7;
			break;
		case C8:
			s_termios.c_cflag |= CS8;
			break;
	}
	switch (parity)
	{
		case NONE:
			break;
		case ODD:
			s_termios.c_cflag |= PARODD;
		case EVEN:
			s_termios.c_cflag |= PARENB;
			break;
	}
	s_termios.c_lflag = 0;
	s_termios.c_cc[VMIN] = getVMin ();
	s_termios.c_cc[VTIME] = getVTime ();

	if (setAttr ())
		return -1;

	tcflush (sock, TCIOFLUSH);
	return 0;
}

int ConnSerial::setVTime (int _vtime)
{
	s_termios.c_cc[VTIME] = _vtime;
	if (setAttr ())
		return -1;

	vTime = _vtime;
	return 0;
}

int ConnSerial::writePort (char ch)
{
	int wlen = 0;
	if (debugPortComm)
	{
		logStream (MESSAGE_DEBUG) << "write char " << std::hex << std::setfill ('0') << std::setw (2)<< ((int) ch) << sendLog;
	}
	while (wlen < 1)
	{
		int ret = write (sock, &ch, 1);
		if (ret == -1 && errno != EINTR)
		{
			logStream (MESSAGE_ERROR) << "cannot write to serial port " << strerror (errno) << sendLog;
			flushError ();
			return -1;
		}
		if (ret == 0)
		{
			logStream (MESSAGE_ERROR) << "write 0 bytes to serial port" << sendLog;
			flushError ();
			return -1;
		}
		wlen += ret;
	}
	return 0;
}

int ConnSerial::writePort (const char *wbuf, int b_len)
{
	int wlen = 0;
	if (debugPortComm)
	{
		LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "will write to port: '";
		logBuffer (ls, wbuf, b_len);
		ls <<  "'" << sendLog;
	}
	while (wlen < b_len)
	{
		int ret = write (sock, wbuf, b_len);
		if (ret == -1 && errno != EINTR)
		{
			logStream (MESSAGE_ERROR) << "cannot write to serial port " << strerror (errno) << sendLog;
			flushError ();
			return -1;
		}
		if (ret == 0)
		{
			logStream (MESSAGE_ERROR) << "write 0 bytes to serial port" << sendLog;
			flushError ();
			return -1;
		}
		wlen += ret;
	}
	return 0;
}

int ConnSerial::readPort (char &ch)
{
	int rlen = 0;
	// it looks max vtime is 100, do not know why..
	int ntries = getVTime () / 100;

	while (rlen == 0)
	{
		rlen = read (sock, &ch, 1);
		if (rlen == -1 && errno != EINTR)
		{
			logStream (MESSAGE_ERROR) << "cannot read single char from serial port, error is " << strerror (errno) << sendLog;
			flushError ();
			return -1;
		}
		if (rlen == 0)
		{
			if (ntries == 0)
			{
				logStream (MESSAGE_ERROR) << "read 0 bytes from serial port" << sendLog;
				flushError ();
				return -1;
			}
			ntries--;
		}
	}
	if (debugPortComm)
	{
		logStream (MESSAGE_DEBUG) << "readed from port 0x" << std::hex << std::setfill ('0') << std::setw(2) << ((int) ch) << sendLog;
	}
	return 1;
}

int ConnSerial::readPort (char *rbuf, int b_len)
{
	int rlen = 0;
	int ntries = getVTime () / 100;
	while (rlen < b_len)
	{
		int ret = read (sock, rbuf + rlen, b_len - rlen);
		if (ret == -1 && errno != EINTR)
		{
			if (rlen > 0)
			{
				rbuf[rlen] = '\0';
				logStream (MESSAGE_ERROR) << "cannot read from serial port after reading '" << rbuf << "', error is " << strerror (errno) << sendLog;
			}
			else
			{
				logStream (MESSAGE_ERROR) << "cannot read from serial port " << strerror (errno) << sendLog;
			}
			flushError ();
			return -1;
		}
		if (ret == 0)
		{
			if (ntries == 0)
			{
				LogStream ls = logStream (MESSAGE_ERROR);
				ls << "read 0 bytes from serial port after reading " << rlen << " bytes sucessfully '";
				logBuffer (ls, rbuf, rlen);
				ls << "'" << sendLog;
				
				flushError ();
				return -1;
			}
			ntries--;
		}

		rlen += ret;
	}
	if (debugPortComm)
	{
		char *tmp_b = new char[rlen + 1];
		memcpy (tmp_b, rbuf, rlen);
		tmp_b[rlen] = '\0';
		LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "readed from port '";
		logBuffer (ls, rbuf, rlen);
		ls << "'" << sendLog;
		delete []tmp_b;
	}
	return rlen;
}

size_t ConnSerial::readPortNoBlock (char *rbuf, size_t b_len)
{
	int old_flags = fcntl (sock, F_GETFL, 0);
	int ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret)
		throw Error (strerror (errno));
	ret = read (sock, rbuf, b_len);
	fcntl (sock, F_SETFL, old_flags);
	if (ret < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		throw Error (strerror (errno));	
	}
	if (debugPortComm)
	{
		LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "readed from port '";
		logBuffer (ls, rbuf, ret);
		ls << "'" << sendLog;
	}
	return ret;
}

int ConnSerial::readPort (char *rbuf, int b_len, char endChar)
{
	int rlen = 0;
	int ntries = getVTime () / 100;
	while (rlen < b_len)
	{
		int ret = read (sock, rbuf + rlen, 1);
		if (ret == -1 && errno != EINTR)
		{
			if (rlen > 0)
			{
				rbuf[rlen] = '\0';
				logStream (MESSAGE_ERROR) << "cannot read from serial port after reading '" << rbuf << "', error is " << strerror (errno) << sendLog;
			}
			else
			{
				logStream (MESSAGE_ERROR) << "cannot read from serial port " << strerror (errno) << sendLog;
			}
			flushError ();
			return -1;
		}
		if (ret == 0)
		{
			if (ntries == 0)
			{
				rbuf[rlen] = '\0';
				logStream (MESSAGE_ERROR) << "readPort with endChar: read 0 bytes from serial port (" << rlen << ":" << rbuf << ")" << sendLog;
				flushError ();
				return -1;
			}
			ntries--;
		}
		if (*(rbuf + rlen) == endChar)
		{
			rlen += ret;
			if (debugPortComm)
			{
				LogStream ls = logStream (MESSAGE_DEBUG);
				ls << "readed from port '";
				logBuffer (ls, rbuf, rlen);
				ls << "'" << sendLog;
			}
			return rlen;
		}
		rlen += ret;
	}
	LogStream ls = logStream (MESSAGE_ERROR);
	ls << "did not find end char '" << endChar
		<< "', readed '";
	logBuffer (ls, rbuf, rlen);
	ls << "'" << sendLog;
	flushError ();
	return -1;
}

int ConnSerial::readPort (char *rbuf, int b_len, const char *endChar)
{
	int tl = 0;
	while (true)
	{
		if ((b_len - tl) < strlen (endChar))
		{
			rbuf[tl] = '\0';
			logStream (MESSAGE_ERROR) << "too few space in read buffer, so far readed " << rbuf << sendLog;
			flushError ();
			return -1;
		}
		// look for the first character
		int ret = readPort (rbuf + tl, b_len - tl, *endChar);
		if (ret < 0)
			return ret;
		tl += ret;
		// look for next characters..
		const char *ch = endChar + 1;
		char *rl = rbuf + tl;
		while (true)
		{
			// readed till end..
		 	if (*ch == '\0')
			{
				rbuf[tl - strlen (endChar)] = '\0';
				return tl;
			}
			if (readPort (rl, 1) != 1)
			{
				*rl = '\0';
				logStream (MESSAGE_ERROR) << "cannot read single character while looking for end " << endChar << ", readed " << rbuf << sendLog;
			}
			tl++;
			if (*rl != *ch)
				continue;
			ch++;
		}
	}
}

void ConnSerial::dropDTR ()
{
	int ret;
	// get current state of control signals
	ioctl (sock, TIOCMGET, &ret);

	// Drop DTR
	ret &= ~TIOCM_DTR;
	ioctl (sock, TIOCMSET, &ret);
}

std::string ConnSerial::getModemBits ()
{
	int flags;
	std::string ret;
	if (ioctl (sock, TIOCMGET, &flags))
	{
		throw rts2core::Error ("cannot get modem status");
	}
	if (flags & TIOCM_LE)
		ret += "DSR ";
	if (flags & TIOCM_DTR)
		ret += "DTR ";
	if (flags & TIOCM_RTS)
		ret += "RTS ";
	if (flags & TIOCM_ST)
		ret += "TXD ";
	if (flags & TIOCM_SR)
	 	ret += "RXD ";
	if (flags & TIOCM_CTS)
		ret += "CTS ";
	if (flags & TIOCM_CAR)
		ret += "DCD ";
	if (flags & TIOCM_RNG)
		ret += "RNG ";
	if (flags & TIOCM_DSR)
		ret += "DSR ";

	if (ret.length ())
		ret = ret.substr (0, ret.length () - 1);
	return ret;
}

int ConnSerial::writeRead (const char* wbuf, int wlen, char *rbuf, int rlen)
{
	int ret;
	ret = writePort (wbuf, wlen);
	if (ret < 0)
		return -1;
	return readPort (rbuf, rlen);
}

int ConnSerial::writeRead (const char* wbuf, int wlen, char *rbuf, int rlen, char endChar)
{
	int ret;
	ret = writePort (wbuf, wlen);
	if (ret < 0)
		return -1;
	return readPort (rbuf, rlen, endChar);
}

int ConnSerial::writeRead (const char* wbuf, int wlen, char *rbuf, int rlen, const char *endChar)
{
	int ret;
	ret = writePort (wbuf, wlen);
	if (ret < 0)
		return -1;
	return readPort (rbuf, rlen, endChar);
}

int ConnSerial::flushPortIO ()
{
	return tcflush (sock, TCIOFLUSH);
}

int ConnSerial::flushPortO ()
{
	return tcflush (sock, TCOFLUSH);
}
