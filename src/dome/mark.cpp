/* 
 * Driver for Mark copula.
 * Copyright (C) 2005,2008 Petr Kubanek <petr@kubanek.net>
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

#include "cupola.h"

#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define SLAVE     0x01

#define REG_READ    0x03
#define REG_WRITE   0x06

#define REG_STATE   0x01
#define REG_POSITION    0x02
#define REG_CUP_CONTROL   0x03
#define REG_SPLIT_CONTROL 0x04

// average az size of one step (in arcdeg)
#define STEP_AZ_SIZE    1

// AZ offset - AZ of 0 position (in libnova notation - 0 is South, 90 is West)
#define STEP_AZ_OFFSET    270

// how many deg to turn before switching from fast to non-fast mode
#define FAST_TIMEOUT    1

// how many arcdeg we can be off to be considerd to hit position
#define MIN_ERR     0.6

using namespace rts2dome;

namespace rts2dome
{

/*!
 * Cupola for MARK telescope on Prague Observatory - http://www.observatory.cz
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Mark:public Cupola
{
	private:
		const char *device_file;
		int cop_desc;

		// communication functions
		int write_read (char *w_buf, int w_buf_len, char *r_buf, int r_buf_len);
		int readReg (int reg_id, uint16_t * reg_val);
		int writeReg (int reg_id, uint16_t reg_val);

		double lastFast;

		enum
		{ UNKNOW, IN_PROGRESS, DONE, ERROR }
		initialized;

		int slew ();
		void parkCupola ();
	protected:
		virtual int moveStart ();
		virtual long isMoving ();
		virtual int moveEnd ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();


	public:
		Mark (int argc, char **argv);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int idle ();

		virtual int info ();

		virtual int moveStop ();

		// park copula
		virtual int standby ();
		virtual int off ();

		virtual double getSlitWidth (__attribute__ ((unused)) double alt)
		{
			return 5;
		}
};

}

// last 2 bits are reserved for crc! They are counted in w_buf_len and r_buf_len, and
// will be modified by this function
int
Mark::write_read (char *w_buf, int w_buf_len, char *r_buf,
int r_buf_len)
{
	int ret;
	uint16_t crc16;
	crc16 = getMsgBufCRC16 (w_buf, w_buf_len - 2);
	w_buf[w_buf_len - 2] = (crc16 & 0x00ff);
	w_buf[w_buf_len - 1] = (crc16 & 0xff00) >> 8;
	ret = write (cop_desc, w_buf, w_buf_len);
	#ifdef DEBUG_ALL
	for (int i = 0; i < w_buf_len; i++)
	{
		logStream (MESSAGE_DEBUG) <<
			"Mark::write_read write byte " << i << "value " <<
			w_buf[i] << sendLog;
	}
	#endif
	if (ret != w_buf_len)
	{
		logStream (MESSAGE_ERROR) <<
			"Mark::write_read ret != w_buf_len " << ret << " " <<
			w_buf_len << sendLog;
		return -1;
	}
	ret = read (cop_desc, r_buf, r_buf_len);
	#ifdef DEBUG_ALL
	for (int i = 0; i < r_buf_len; i++)
	{
		logStream (MESSAGE_DEBUG) <<
			"Mark::write_read read byte " << i << " value " <<
			r_buf[i] << sendLog;
	}
	#endif
	if (ret != r_buf_len)
	{
		logStream (MESSAGE_ERROR) <<
			"Mark::write_read ret != r_buf_len " << ret << " " <<
			r_buf_len << sendLog;
		return -1;
	}
	// get checksum
	crc16 = getMsgBufCRC16 (r_buf, r_buf_len - 2);
	if ((r_buf[r_buf_len - 1] & ((crc16 & 0xff00) >> 8)) !=
		((crc16 & 0xff00) >> 8)
		|| (r_buf[r_buf_len - 2] & (crc16 & 0x00ff)) != (crc16 & 0x00ff))
	{
		logStream (MESSAGE_ERROR) <<
			"Mark::write_read invalid checksum! (should be " <<
			((crc16 & 0xff00) >> 8) << " " << (crc16 & 0x00ff) << " is " <<
			r_buf[r_buf_len - 1] << " " << r_buf[r_buf_len - 2] << " ( " <<
			(r_buf[r_buf_len - 1] & ((crc16 & 0xff00) >> 8)) << " " <<
			(r_buf[r_buf_len - 2] & (crc16 & 0x00ff)) << " ))" << sendLog;
		return -1;
	}
	usleep (USEC_SEC / 10);
	return 0;
}


int
Mark::readReg (int reg, uint16_t * reg_val)
{
	int ret;
	char wbuf[8];
	char rbuf[7];
	wbuf[0] = SLAVE;
	wbuf[1] = REG_READ;
	wbuf[2] = (reg & 0xff00) >> 8;
	wbuf[3] = (reg & 0x00ff);
	wbuf[4] = 0x00;
	wbuf[5] = 0x01;
	ret = write_read (wbuf, 8, rbuf, 7);
	if (ret)
		return ret;
	*reg_val = rbuf[3];
	*reg_val = *reg_val << 8;
	*reg_val |= (rbuf[4]);
	logStream (MESSAGE_DEBUG) << "Mark::readReg reg " << reg <<
		" val " << *reg_val << sendLog;
	return 0;
}


int
Mark::writeReg (int reg, uint16_t reg_val)
{
	char wbuf[8];
	char rbuf[8];
	// we must either be initialized or
	// or we must issue initialization request
	if (!(initialized == DONE || (reg == REG_CUP_CONTROL && reg_val == 0x08)))
		return -1;
	wbuf[0] = SLAVE;
	wbuf[1] = REG_WRITE;
	wbuf[2] = (reg & 0xff00) >> 8;
	wbuf[3] = (reg & 0x00ff);
	wbuf[4] = (reg_val & 0xff00) >> 8;
	wbuf[5] = (reg_val & 0x00ff);
	logStream (MESSAGE_DEBUG) << "Mark::writeReg reg " << reg <<
		" value " << reg_val << sendLog;
	return write_read (wbuf, 8, rbuf, 8);
}


Mark::Mark (int in_argc, char **in_argv):Cupola (in_argc,
in_argv)
{
	lastFast = NAN;

	device_file = "/dev/ttyS0";

	initialized = UNKNOW;

	addOption ('f', "device", 1, "device filename (default to /dev/ttyS0");
}


int
Mark::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Cupola::processOption (in_opt);
	}
	return 0;
}


int
Mark::init ()
{
	struct termios cop_termios;
	int ret;
	ret = Cupola::init ();
	if (ret)
		return ret;

	logStream (MESSAGE_DEBUG) << "Mark::init open: " << device_file
		<< sendLog;

	cop_desc = open (device_file, O_RDWR);
	if (cop_desc < 0)
		return -1;

	if (tcgetattr (cop_desc, &cop_termios) < 0)
		return -1;

	if (cfsetospeed (&cop_termios, B4800) < 0 ||
		cfsetispeed (&cop_termios, B4800) < 0)
		return -1;

	cop_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
	cop_termios.c_oflag = 0;
	cop_termios.c_cflag =
		((cop_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
	cop_termios.c_lflag = 0;
	cop_termios.c_cc[VMIN] = 0;
	cop_termios.c_cc[VTIME] = 40;

	if (tcsetattr (cop_desc, TCSANOW, &cop_termios) < 0)
		return -1;

	setTimeout (USEC_SEC);

	return 0;
}


int
Mark::idle ()
{
	int ret;
	uint16_t copState;
	if (initialized == UNKNOW || initialized == DONE
		|| initialized == IN_PROGRESS || initialized == ERROR)
	{
		// check if initialized
		ret = readReg (REG_STATE, &copState);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cupola::idle cannot read reg!"
				<< sendLog;
			sleep (2);
			tcflush (cop_desc, TCIOFLUSH);
			initialized = ERROR;
		}
		else if ((copState & 0x0200) == 0)
		{
			if (initialized != IN_PROGRESS && initialized != DONE)
			{
				// not initialized, initialize
				writeReg (REG_CUP_CONTROL, 0x08);
				initialized = IN_PROGRESS;
			}
		}
		else
		{
			initialized = DONE;
		}
	}
	return Cupola::idle ();
}


int
Mark::startOpen ()
{
	int ret;
	ret = writeReg (REG_SPLIT_CONTROL, 0x0001);
	if (ret)
		return ret;
	return 0;
}


long
Mark::isOpened ()
{
	return -2;
}


int
Mark::endOpen ()
{
	return 0;
}


int
Mark::startClose ()
{
	int ret;
	ret = writeReg (REG_SPLIT_CONTROL, 0x0000);
	if (ret)
		return ret;
	return 0;
}


long
Mark::isClosed ()
{
	int ret;
	uint16_t reg_val;
	ret = readReg (REG_STATE, &reg_val);
	if (ret)
		return ret;
	// 5 bit is split state
	if (reg_val & 0x10)
	{
		// still open
		return USEC_SEC;
	}
	return -2;
}


int
Mark::endClose ()
{
	return 0;
}


int
Mark::info ()
{
	int ret;
	int16_t az_val;
	ret = readReg (REG_POSITION, (uint16_t *) & az_val);
	if (!ret)
	{
		setCurrentAz (ln_range_degrees
			((az_val * STEP_AZ_SIZE) + STEP_AZ_OFFSET));
		return Cupola::info ();
	}
	return Cupola::info ();
}


/**
 * @return -1 on error, -2 when move complete (on target position), 0 when nothing interesting happen
 */
int
Mark::slew ()
{
	int ret;
	uint16_t cupControl;

	if (initialized != DONE)
		return -1;

	ret = readReg (REG_CUP_CONTROL, &cupControl);
	// we need to move in oposite direction..do slow change
	if (((cupControl & 0x01) && getTargetDistance () < 0)
		|| ((cupControl & 0x02) && getTargetDistance () > 0))
	{
		// stop faster movement..
		if (cupControl & 0x04)
		{
			cupControl &= ~(0x04);
			lastFast = getCurrentAz ();
			return writeReg (REG_CUP_CONTROL, cupControl);
		}
		// wait for copula to reach some distance in slow mode..
		if (fabs (lastFast - getCurrentAz ()) < FAST_TIMEOUT
			|| fabs (lastFast - getCurrentAz ()) > (360 - FAST_TIMEOUT))
		{
			return 0;
		}
		// can finaly stop movement..
		cupControl &= ~(0x03);
		writeReg (REG_CUP_CONTROL, cupControl);
		lastFast = NAN;
	}
	// going our direction..
	if ((cupControl & 0x01) || (cupControl & 0x02))
	{
		// already in fast mode..
		if (cupControl & 0x04)
		{
			// try to slow down when needed
			if (fabs (getTargetDistance ()) < FAST_TIMEOUT)
			{
				cupControl &= ~(0x04);
				ret = writeReg (REG_CUP_CONTROL, cupControl);
				if (ret)
					return -1;
			}
			logStream (MESSAGE_DEBUG) << "Mark::slew slow down" <<
				sendLog;
		}
		// test if we hit target destination
		if (fabs (getTargetDistance ()) < MIN_ERR)
		{
			cupControl &= ~(0x03);
			ret = writeReg (REG_CUP_CONTROL, cupControl);
			if (ret)
				return -1;
			return -2;
		}
	}
	// not move at all
	if ((cupControl & 0x03) == 0x00)
	{
		cupControl = (getTargetDistance () < 0) ? 0x02 : 0x01;
		ret = writeReg (REG_CUP_CONTROL, cupControl);
		if (ret)
			return -1;
		return 0;
	}
	return 0;
}


int
Mark::moveStart ()
{
	int ret;
	ret = needSlitChange ();
	if (ret == 0 || ret == -1)
		return ret;				 // pretend we change..so other devices can sync on our command
	slew ();
	return Cupola::moveStart ();
}


long
Mark::isMoving ()
{
	int ret;
	ret = needSlitChange ();
	if (ret == -1)
		return ret;
	ret = slew ();
	if (ret == 0)
		return USEC_SEC / 100;
	return ret;
}


int
Mark::moveEnd ()
{
	writeReg (REG_CUP_CONTROL, 0x00);
	return Cupola::moveEnd ();
}


int
Mark::moveStop ()
{
	writeReg (REG_CUP_CONTROL, 0x00);
	return Cupola::moveStop ();
}


void
Mark::parkCupola ()
{
	if (initialized != IN_PROGRESS)
	{
		// reinitialize copula
		writeReg (REG_CUP_CONTROL, 0x08);
		initialized = IN_PROGRESS;
	}
}

int Mark::standby ()
{
	parkCupola ();
	return Cupola::standby ();
}

int Mark::off ()
{
	parkCupola ();
	return Cupola::off ();
}

int main (int argc, char **argv)
{
	Mark device (argc, argv);
	return device.run ();
}
