/* 
 * Support for Optec focusers.
 * Copyright (C) 2004-2006 Stanislav Vitek <standa@iaa.es>
 * Copyright (C) 2004-2007 Petr Kubanek <petr@kubanek.net>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#define BAUDRATE B19200
#define FOCUSER_PORT "/dev/ttyS0"

#include "focuser.h"

class Rts2DevFocuserOptec:public Rts2DevFocuser
{
	private:
		char *device_file_io;
		int foc_desc;
		int status;
		bool damagedTempSens;

		Rts2ValueFloat *focTemp;

		// low-level I/O functions
		int foc_read (char *buf, int count, int timeouts);
		int foc_write (char *buf, int count);
		int foc_write_read (char *wbuf, int wcount, char *rbuf, int rcount,
			int timeouts = 0);

		// high-level I/O functions
		int getPos (Rts2ValueInteger * position);
		int getTemp ();
	protected:
		virtual bool isAtStartPosition ();
	public:
		Rts2DevFocuserOptec (int argc, char **argv);
		~Rts2DevFocuserOptec (void);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int ready ();
		virtual int initValues ();
		virtual int info ();
		virtual int stepOut (int num);
		virtual int isFocusing ();
};

/*!
 * Reads some data directly from port.
 *
 * Log all flow as LOG_DEBUG to syslog
 *
 * @exception EIO when there aren't data from port
 *
 * @param buf           buffer to read in data
 * @param count         how much data will be readed
 *
 * @return -1 on failure, otherwise number of read data
 */

int
Rts2DevFocuserOptec::foc_read (char *buf, int count, int timeouts)
{
	int readed;

	readed = 0;

	while (readed < count)
	{
		int ret;
		ret = read (foc_desc, &buf[readed], count - readed);
		if (ret <= 0)
		{
			if (timeouts <= 0)
			{
				logStream (MESSAGE_ERROR) << "focuser Optec foc_read " <<
					strerror (errno) << " (" << errno << ")" << sendLog;
				return -1;
			}
			timeouts--;
			// repeat read
			ret = 0;
		}
		readed += ret;
	}
	return readed;
}


/*!
 * Will write on telescope port string.
 *
 * @exception EIO, .. common write exceptions
 *
 * @param buf           buffer to write
 * @param count         count to write
 *
 * @return -1 on failure, count otherwise
 */
int
Rts2DevFocuserOptec::foc_write (char *buf, int count)
{
	int ret;
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "focuser Optec will write: " << buf << sendLog;
	#endif
	ret = write (foc_desc, buf, count);
	//  tcflush (foc_desc, TCIFLUSH);
	return ret;
}


/*!
 * Combine write && read together.
 *
 * Flush port to clear any gargabe.
 *
 * @exception EINVAL and other exceptions
 *
 * @param wbuf          buffer to write on port
 * @param wcount        write count
 * @param rbuf          buffer to read from port
 * @param rcount        maximal number of characters to read
 *
 * @return -1 and set errno on failure, rcount otherwise
 */
int
Rts2DevFocuserOptec::foc_write_read (char *wbuf, int wcount, char *rbuf,
int rcount, int timeouts)
{
	int tmp_rcount = -1;
	if (foc_write (wbuf, wcount) < 0)
		return -1;

	tmp_rcount = foc_read (rbuf, rcount, timeouts);

	if (tmp_rcount > 0)
	{
		#ifdef DEBUG_EXTRA
		char *buf;
		buf = (char *) malloc (rcount + 1);
		memcpy (buf, rbuf, rcount);
		buf[rcount] = '\0';
		logStream (MESSAGE_DEBUG) << "focuser Optec readed " << tmp_rcount <<
			" " << buf << sendLog;
		free (buf);
		#endif
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "focuser Optec readed returns " <<
			tmp_rcount << sendLog;
	}
	return tmp_rcount;
}


Rts2DevFocuserOptec::Rts2DevFocuserOptec (int in_argc, char **in_argv):Rts2DevFocuser (in_argc,
in_argv)
{
	device_file = FOCUSER_PORT;
	damagedTempSens = false;
	focTemp = NULL;

	addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
	addOption ('D', "damaged_temp_sensor", 0,
		"if focuser have damaged temp sensor");
}


Rts2DevFocuserOptec::~Rts2DevFocuserOptec ()
{
	close (foc_desc);
}


int
Rts2DevFocuserOptec::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case 'D':
			damagedTempSens = true;
			break;
		default:
			return Rts2DevFocuser::processOption (in_opt);
	}
	return 0;
}


/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int
Rts2DevFocuserOptec::init ()
{
	struct termios foc_termios;
	char rbuf[10];
	int ret;

	ret = Rts2DevFocuser::init ();

	if (ret)
		return ret;

	if (!damagedTempSens)
	{
		createFocTemp ();
		createValue (focTemp, "FOC_TEMP", "focuser temperature");
	}

	foc_desc = open (device_file, O_RDWR);

	if (foc_desc < 0)
		return -1;

	if (tcgetattr (foc_desc, &foc_termios) < 0)
		return -1;

	if (cfsetospeed (&foc_termios, BAUDRATE) < 0 ||
		cfsetispeed (&foc_termios, BAUDRATE) < 0)
		return -1;

	foc_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
	foc_termios.c_oflag = 0;
	foc_termios.c_cflag =
		((foc_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
	foc_termios.c_lflag = 0;
	foc_termios.c_cc[VMIN] = 0;
	foc_termios.c_cc[VTIME] = 40;

	if (tcsetattr (foc_desc, TCSANOW, &foc_termios) < 0)
		return -1;

	tcflush (foc_desc, TCIOFLUSH);

	// set manual
	if (foc_write_read ("FMMODE", 6, rbuf, 3) < 0)
		return -1;
	if (rbuf[0] != '!')
		return -1;

	ret = checkStartPosition ();

	return ret;
}


int
Rts2DevFocuserOptec::getPos (Rts2ValueInteger * position)
{
	char rbuf[9];

	if (foc_write_read ("FPOSRO", 6, rbuf, 8) < 1)
		return -1;
	else
	{
		rbuf[6] = '\0';
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "0: " << rbuf[0] << sendLog;
		#endif
		position->setValueInteger (atoi ((rbuf + 2)));
	}
	return 0;
}


int
Rts2DevFocuserOptec::getTemp ()
{
	char rbuf[10];

	if (damagedTempSens)
		return 0;

	if (foc_write_read ("FTMPRO", 6, rbuf, 9) < 1)
		return -1;
	else
	{
		rbuf[7] = '\0';
		focTemp->setValueFloat (atof ((rbuf + 2)));
	}
	return 0;
}


bool Rts2DevFocuserOptec::isAtStartPosition ()
{
	int
		ret;
	ret = getPos (focPos);
	if (ret)
		return false;
	return (getFocPos () == 3500);
}


int
Rts2DevFocuserOptec::ready ()
{
	return 0;
}


int
Rts2DevFocuserOptec::initValues ()
{
	strcpy (focType, "OPTEC_TCF");
	return Rts2DevFocuser::initValues ();
}


int
Rts2DevFocuserOptec::info ()
{
	int ret;
	ret = getPos (focPos);
	if (ret)
		return ret;
	ret = getTemp ();
	if (ret)
		return ret;
	return Rts2DevFocuser::info ();
}


int
Rts2DevFocuserOptec::stepOut (int num)
{
	char command[7], rbuf[4];
	char add = ' ';
	int ret;

	ret = getPos (focPos);
	if (ret)
		return ret;

	if (getFocPos () + num > 7000 || getFocPos () + num < 0)
		return -1;

	if (num < 0)
	{
		add = 'I';
		num *= -1;
	}
	else
	{
		add = 'O';
	}

	// maximal time fore move is +- 40 sec

	sprintf (command, "F%c%04d", add, num);

	if (foc_write_read (command, 6, rbuf, 3, 10) < 0)
		return -1;
	if (rbuf[0] != '*')
		return -1;

	return 0;
}


int
Rts2DevFocuserOptec::isFocusing ()
{
	// stepout command waits till focusing end
	return -2;
}


int
main (int argc, char **argv)
{
	Rts2DevFocuserOptec device = Rts2DevFocuserOptec (argc, argv);
	return device.run ();
}
