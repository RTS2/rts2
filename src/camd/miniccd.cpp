/* 
 * Driver for Starlight Xpress CCDs.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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
#include <errno.h>

#include "camd.h"
#include "ccd_msg.h"
#include "../utils/rts2device.h"

class Rts2DevCameraMiniccd:public Rts2DevCamera
{
	private:
		int fd_ccd;
		int fd_chip;
		int interleave;
		char *device_file;

		int ccd_dac_bits;
		int sizeof_pixel;

		CCD_ELEM_TYPE msgw[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];
		CCD_ELEM_TYPE msgr[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int initChips ();
		virtual void initDataTypes ();

		virtual int startExposure ();
		virtual long isExposing ();
		virtual int stopExposure ();
		virtual int readoutOneLine ();
	public:
		Rts2DevCameraMiniccd (int argc, char **argv);
		virtual ~ Rts2DevCameraMiniccd (void);

		virtual int ready ();
		virtual int camCoolMax ();
		virtual int camCoolHold ();
		virtual int setCoolTemp (float new_temp);
		virtual int camCoolShutdown ();
		virtual int camFilter (int new_filter);
};

int
Rts2DevCameraMiniccd::initChips ()
{
	int in_width;
	int in_height;

	int msg_len;

	msg_len = strlen (device_file);

	char chip_name[msg_len + 2];
	strcpy (chip_name, device_file);
	chip_name[msg_len] = '1';
	chip_name[msg_len + 1] = '\0';

	fd_chip = open (chip_name, O_RDWR);
	if (fd_chip < 0)
	{
		logStream (MESSAGE_ERROR)
			<< "Rts2DevCameraMiniccd::initChips cannot open device file: " <<
			strerror (errno) << sendLog;
		return -1;
	}

	msgw[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msgw[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_QUERY_LEN;
	msgw[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msgw[CCD_MSG_INDEX] = CCD_MSG_QUERY;

	write (fd_chip, (char *) msgw, CCD_MSG_QUERY_LEN);
	msg_len = read (fd_chip, (char *) msgr, CCD_MSG_CCD_LEN);

	if (msg_len != CCD_MSG_CCD_LEN)
	{
		logStream (MESSAGE_ERROR)
			<< "Rts2DevCameraMiniccd::init CCD message length wrong: "
			<< msg_len << " " << strerror (errno) << sendLog;
		return -1;
	}

	in_width = msgr[CCD_CCD_WIDTH_INDEX];
	in_height = msgr[CCD_CCD_HEIGHT_INDEX];

	sizeof_pixel = (msgr[CCD_CCD_DEPTH_INDEX] + 7) / 8;

	setSize (in_width, in_height, 0, 0);

	ccd_dac_bits = msgr[CCD_CCD_DAC_INDEX];

	return 0;
}


void
Rts2DevCameraMiniccd::initDataTypes ()
{
	switch (sizeof_pixel)
	{
		case 1:
			addDataType (RTS2_DATA_BYTE);
			break;
		case 2:
			addDataType (RTS2_DATA_USHORT);
			break;
	}
}


int
Rts2DevCameraMiniccd::startExposure ()
{
	CCD_ELEM_TYPE msg[CCD_MSG_EXP_LEN / CCD_ELEM_SIZE];
	int exposure_msec = (int) (getExposure () * 1000);
	int ccdFlags = getExpType () ? CCD_EXP_FLAGS_NOOPEN_SHUTTER: 0;
	int ret;

	msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_EXP_LEN;
	msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msg[CCD_MSG_INDEX] = CCD_MSG_EXP;
	msg[CCD_EXP_WIDTH_INDEX] = getUsedWidth ();
	msg[CCD_EXP_HEIGHT_INDEX] = getUsedHeight ();
	msg[CCD_EXP_XOFFSET_INDEX] = chipTopX ();
	msg[CCD_EXP_YOFFSET_INDEX] = chipTopY ();
	msg[CCD_EXP_XBIN_INDEX] = binningHorizontal ();
	msg[CCD_EXP_YBIN_INDEX] = binningVertical ();
	msg[CCD_EXP_DAC_INDEX] = ccd_dac_bits;
	msg[CCD_EXP_FLAGS_INDEX] = ccdFlags;
	msg[CCD_EXP_MSEC_LO_INDEX] = exposure_msec & 0xFFFF;
	msg[CCD_EXP_MSEC_HI_INDEX] = exposure_msec >> 16;
	ret = write (fd_chip, (char *) msg, CCD_MSG_EXP_LEN);
	if (ret == CCD_MSG_EXP_LEN)
	{
		return 0;
	}
	return -1;
}


long
Rts2DevCameraMiniccd::isExposing ()
{
	fd_set set;
	struct timeval read_tout;
	long ret;

	ret = Rts2DevCamera::isExposing ();
	if (ret > 0)
		return ret;

	FD_ZERO (&set);
	FD_SET (fd_chip, &set);
	read_tout.tv_sec = read_tout.tv_usec = 0;
	select (fd_chip + 1, &set, NULL, NULL, &read_tout);
	if (FD_ISSET (fd_chip, &set))
	{
		CCD_ELEM_TYPE msgi[CCD_MSG_IMAGE_LEN];
		ret = read (fd_chip, msgi, CCD_MSG_IMAGE_LEN);
		if (ret != CCD_MSG_IMAGE_LEN)
			return -1;
		// test image header
		if (msgi[CCD_MSG_INDEX] != CCD_MSG_IMAGE)
		{
			logStream (MESSAGE_ERROR) <<
				"miniccd sendLineData wrong image message" << sendLog;
			return -1;
		}
		if ((unsigned int) (msgi[CCD_MSG_LENGTH_LO_INDEX] + (msgi[CCD_MSG_LENGTH_HI_INDEX] << 16))
			!= (chipByteSize () + CCD_MSG_IMAGE_LEN))
		{
			logStream (MESSAGE_ERROR) << "miniccd sendLineData wrong size " <<
				msgi[CCD_MSG_LENGTH_LO_INDEX] +
				(msgi[CCD_MSG_LENGTH_HI_INDEX] << 16) << sendLog;
			return -1;
		}
		return 0;
	}
	return 100;
}


int
Rts2DevCameraMiniccd::readoutOneLine ()
{
	int ret;

	ret = read (fd_chip, dataBuffer, getWriteBinaryDataSize ());
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "Error during reading data" << sendLog;
		return -1;
	}

	ret = sendReadoutData (dataBuffer, ret);
	if (ret < 0)
		return ret;

	if (getWriteBinaryDataSize () == 0)
		return -2;
	return 0;
}


int
Rts2DevCameraMiniccd::stopExposure ()
{
	CCD_ELEM_TYPE msg[CCD_MSG_ABORT_LEN / CCD_ELEM_SIZE];
	/*
	 * Send the abort request.
	 */
	msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_ABORT_LEN;
	msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msg[CCD_MSG_INDEX] = CCD_MSG_ABORT;
	write (fd_chip, (char *) msg, CCD_MSG_ABORT_LEN);
	return Rts2DevCamera::stopExposure ();
}


Rts2DevCameraMiniccd::Rts2DevCameraMiniccd (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
	fd_ccd = -1;
	device_file = NULL;

	createExpType ();

	addOption ('f', "device_file", 1, "miniccd device file (/dev/xxx entry)");
}


Rts2DevCameraMiniccd::~Rts2DevCameraMiniccd (void)
{
	close (fd_ccd);
}


int
Rts2DevCameraMiniccd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Rts2DevCamera::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevCameraMiniccd::init ()
{
	int ret;
	int msg_len;

	ret = Rts2DevCamera::init ();
	if (ret)
		return ret;

	fd_ccd = open (device_file, O_RDWR);
	if (fd_ccd < 0)
	{
		logStream (MESSAGE_ERROR)
			<< "Rts2DevCameraMiniccd::init Unable to open device: "
			<< device_file << " " << strerror (errno) << sendLog;
		return -1;
	}
	/*
	 * Request CCD parameters.
	 */
	msgw[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msgw[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_QUERY_LEN;
	msgw[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msgw[CCD_MSG_INDEX] = CCD_MSG_QUERY;
	write (fd_ccd, (char *) msgw, CCD_MSG_QUERY_LEN);
	msg_len = read (fd_ccd, (char *) msgr, CCD_MSG_CCD_LEN);
	if (msg_len != CCD_MSG_CCD_LEN)
	{
		logStream (MESSAGE_ERROR) << "miniccd init CCD message length wrong " <<
			msg_len << sendLog;
		return -1;
	}
	/*
	 * Response from CCD query.
	 */
	if (msgr[CCD_MSG_INDEX] != CCD_MSG_CCD)
	{
		logStream (MESSAGE_ERROR) <<
			"miniccd init Wrong message returned from query " <<
			msgr[CCD_MSG_INDEX] << sendLog;
		return -1;
	}

	strncpy (ccdType, (char *) &msgr[CCD_CCD_NAME_INDEX], CCD_CCD_NAME_LEN / 2);

	char *top = ccdType;

	while ((isgraph (*top) || isspace (*top))
		&& top - ccdType < CCD_CCD_NAME_LEN / 2)
		top++;
	*top = '\0';

	return initChips ();
}


int
Rts2DevCameraMiniccd::ready ()
{
	return !(fd_ccd != -1);
}


int
Rts2DevCameraMiniccd::camCoolMax ()
{
	CCD_ELEM_TYPE msg[CCD_MSG_TEMP_LEN / CCD_ELEM_SIZE];

	msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_TEMP_LEN;
	msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msg[CCD_MSG_INDEX] = CCD_MSG_TEMP;
	msg[CCD_TEMP_SET_HI_INDEX] = 0;
	msg[CCD_TEMP_SET_LO_INDEX] = 0;
	write (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
	read (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
	return 0;
}


int
Rts2DevCameraMiniccd::camCoolHold ()
{
	return 0;
}


int
Rts2DevCameraMiniccd::setCoolTemp (float new_temp)
{
	return 0;
}


int
Rts2DevCameraMiniccd::camCoolShutdown ()
{
	return 0;
}


int
Rts2DevCameraMiniccd::camFilter (int in_filter)
{
	return 0;
}


int
main (int argc, char **argv)
{
	Rts2DevCameraMiniccd device = Rts2DevCameraMiniccd (argc, argv);
	return device.run ();
}
