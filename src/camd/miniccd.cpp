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

/*		int startExposureInterleaved (int light, float exptime, int ccdFlags = 0)
		{
			if (exptime < 10)
				return -1;
			return startExposure (light, exptime, ccdFlags, 1);
		}
		virtual int startExposure (int light, float exptime)
		{
			return startExposure (light, exptime, 0, 0);
		} */

/*char *
CameraMiniccdChip::getAllData ()
{
	char *ret;

	if (sendLine == 0)
	{
		// test data size & ignore data header
		CCD_ELEM_TYPE *msg = (CCD_ELEM_TYPE *) _data;
		if (msg[CCD_MSG_INDEX] != CCD_MSG_IMAGE)
		{
			logStream (MESSAGE_ERROR) <<
				"miniccd getAllData wrong image message" << sendLog;
			return NULL;
		}
		if (!chipUsedReadout)
		{
			logStream (MESSAGE_ERROR) <<
				"miniccd getAllData not chipUsedReadout" << sendLog;
			return NULL;
		}
		if ((unsigned int) (msg[CCD_MSG_LENGTH_LO_INDEX] +
			(msg[CCD_MSG_LENGTH_HI_INDEX] << 16)) !=
			((chipUsedReadout->height / usedBinningVertical) *
			usedRowBytes) + CCD_MSG_IMAGE_LEN)
		{
			logStream (MESSAGE_ERROR) << "miniccd getAllData wrong size " <<
				msg[CCD_MSG_LENGTH_LO_INDEX] +
				(msg[CCD_MSG_LENGTH_HI_INDEX] << 16) << sendLog;
			return NULL;
		}
		send_top += CCD_MSG_IMAGE_LEN;
	}
	sendLine++;
	ret = send_top;
	send_top += usedRowBytes;
	return ret;
}*/

/*
class CameraMiniccdInterleavedChip:public CameraChip
{
	private:
		enum
		{
			NO_ACTION, SLAVE1_EXPOSING, SLAVE2_EXPOSING, SLAVE1_READOUT,
			SLAVE2_READOUT, SENDING
		} slaveState;
		CameraMiniccdChip *slaveChip[2];
		long firstReadoutTime;
		struct timeval slave1ReadoutStart;
		struct timeval slave2ExposureStart;
		int chip1_light;
		float chip1_exptime;
		int usedRowBytes;
		// do 2x2 binning
		char *doBinning (uint16_t * row1, uint16_t * row2);
		char *send_top;
		char *dest_top;
		uint16_t *_data;
	public:
		CameraMiniccdInterleavedChip (Rts2DevCamera * in_cam, int in_chip_id,
			int in_fd_chip,
			CameraMiniccdChip * in_chip1,
			CameraMiniccdChip * in_chip2);
		virtual ~ CameraMiniccdInterleavedChip (void);
		virtual int init ();
		virtual int startExposure (int light, float exptime);
		virtual long isExposing ();
		virtual int stopExposure ();
		virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
		virtual int endReadout ();
		virtual int readoutOneLine ();
};

CameraMiniccdInterleavedChip::CameraMiniccdInterleavedChip (Rts2DevCamera * in_cam,
int in_chip_id, int in_fd_chip, CameraMiniccdChip * in_chip1, CameraMiniccdChip * in_chip2):
CameraChip (in_cam, in_chip_id)
{
	slaveChip[0] = in_chip1;
	slaveChip[1] = in_chip2;
								 // default readout is expected to last 3 sec..
	firstReadoutTime = 3 * USEC_SEC;
	slaveState = NO_ACTION;
	_data = NULL;
}

CameraMiniccdInterleavedChip::~CameraMiniccdInterleavedChip (void)
{
	delete[]_data;
}

char *
CameraMiniccdInterleavedChip::doBinning (uint16_t * row1, uint16_t * row2)
{
	#define SCALE_X   0.5
	#define SCALE_Y   0.259
	// offset aplied after scalling
	#define OFF_X   1
	#define OFF_Y   3
	// pixels which are processed ring now
	uint16_t a0_0, a0_1, a1_0, a1_1;
	uint16_t b0_0, b0_1, b1_0, b1_1;
	int w = chipUsedReadout->width * 2;
	int h = chipUsedReadout->height * 2;
	int x, y, n_x;
	long n_y;
	float n_fx, n_fy;
	int n_w, n_h;
	uint16_t *r_swap;
	// those arrays hold transformed images
	uint16_t *img_1 = new uint16_t[(w + 1) * (h + 1) * 8];
	uint16_t *img_2 = new uint16_t[(w + 1) * (h + 1) * 8];
	uint16_t *img_com = new uint16_t[(w + 1) * (h + 1) * 8];
	// initialize data..
	if (!_data)
	{
		_data = new uint16_t[w * h];
	}
	// split data to odd and even columns, scale by factor 0.5 and 0.259
	// init new coordinates
	n_y = 0;
	n_fy = 0;

	n_w = (int) (w / SCALE_X / 2);

	// end of image..

	// biliniear scalling
	// y is coordinate in old image, n_x and n_y are coordinates in transformed image
	for (y = 0; y < (chipUsedReadout->height - 1); y++)
	{
		a0_0 = row1[0];
		a0_1 = row1[2];
		a1_0 = row2[w];
		a1_1 = row2[w + 2];

		b0_0 = row1[0];
		b0_1 = row1[2];
		b1_0 = row2[w + 1];
		b1_1 = row2[w + 3];

		uint16_t *n_r1 = row1 + (w - 1);
		float fi;
		int i;

		n_x = 0;
		n_fx = 0;

		for (x = 0; row1 < n_r1; x++)
		{
			a0_1 = row1[2];
			a1_1 = row1[w + 2];

			b0_1 = row2[2];
			b1_1 = row2[w + 3];
			float x_fmax = 0;
			int x_max = 0;
			// fill all pixels we can fill with data we have in a and b
			for (fi = n_fy, i = 0; fi < (y + 1); fi += SCALE_Y, i++)
			{
				float fj;
				int j;
				for (fj = n_fx, j = 0; fj < (x + 1) && n_x + j < n_w;
					fj += SCALE_X, j++)
				{
					int index = n_y + n_w * i + (n_x + j);
					// drop to [0,0],[1,1] system..
					float ii = fi - n_fy;
					float jj = fj - n_fx;
					img_1[index] = (uint16_t) (a0_0 * (ii - 1) * (jj - 1)
						- a1_0 * jj * (ii - 1)
						- a0_1 * (jj - 1) * ii
						+ a1_1 * jj * ii);
					img_2[index] = (uint16_t) (b0_0 * (ii - 1) * (jj - 1)
						- b1_0 * jj * (ii - 1)
						- b0_1 * (jj - 1) * ii
						+ b1_1 * jj * ii);
				}
				if (fi == n_fy)
				{
					x_fmax = fj;
					x_max = j;
				}
			}

			row1 += 2;
			row2 += 2;

			n_fx = x_fmax;
			n_x += x_max;

			a0_0 = a0_1;
			a1_0 = a1_1;

			b0_0 = b0_1;
			b1_0 = b1_1;
		}
		n_fy += (fi - n_fy);
		n_y += n_w * i;
		// swap rows..
		r_swap = row1;
		row1 = row2;
		row2 = r_swap;
	}
	n_h = n_y / n_w;

	// ok, we have transformed image, now combine it back to master image..
	for (y = 0; y < n_h; y++)
		for (x = 0; x < n_w; x++)
	{
		int n_x2 = x + OFF_X;
		int n_y2 = y + OFF_Y;
		int index = y * n_w + x;
		if (n_x2 < 0 || n_x2 > n_w || n_y2 < 0 || n_y2 > n_h)
		{
			img_com[index] = img_1[index];
		}
		else
		{
			img_com[index] =
				(uint16_t) (((long) img_1[index] + (long) img_2[index]) / 2);
		}
	}
	// and scale to 1/4 in height and 1/2 in height (as 1/2 of scalling is done by combining rows and cols..
	w = chipUsedReadout->width;
	h = chipUsedReadout->height;
	int n_x2;
	int n_y2;
	for (y = 0, n_y2 = 0; y < h; y++, n_y2 += 4)
		for (x = 0, n_x2 = 0; x < w; x++, n_x2 += 2)
	{
		long sum = 0;
		int num = 0;
		for (int yy = n_y2; yy < n_y2 + 4 && yy < n_h; yy++)
			for (int xx = n_x2; xx < n_x2 + 2 && xx < n_w; xx++)
		{
			sum += img_com[yy * n_w + xx];
			num++;
		}
		if (num > 0)
		{
			_data[y * w + x] = sum / num;
		}
		else
		{
			_data[y * w + x] = 0;
		}
	}
	delete[]img_1;
	delete[]img_2;
	delete[]img_com;
	return (char *) _data;
}

int
CameraMiniccdInterleavedChip::init ()
{
	int in_width;
	int in_height;

	int sizeof_pixel;

	in_width = slaveChip[0]->getWidth ();

	if (in_width != slaveChip[1]->getWidth ())
	{
		logStream (MESSAGE_ERROR) <<
			"miniccd interleaved chip init not same width" << sendLog;
		return -1;
	}

	sizeof_pixel = slaveChip[0]->getSizeOfPixel ();

	if (sizeof_pixel != slaveChip[1]->getSizeOfPixel ())
	{
		logStream (MESSAGE_ERROR) <<
			"miniccd interleaved chip init not same sizeof pixel" << sendLog;
		return -1;
	}

	in_height = slaveChip[0]->getHeight () + slaveChip[1]->getHeight ();

	setSize (in_width, in_height, 0, 0);

	return 0;
}

int
CameraMiniccdInterleavedChip::startExposure (int light, float exptime)
{
	int ret;

	gettimeofday (&slave2ExposureStart, NULL);

	// distribute changes to chipReadout structure..
	slaveChip[0]->box (chipReadout->x, chipReadout->y, chipReadout->width,
		chipReadout->height / 2);
	slaveChip[1]->box (chipReadout->x, chipReadout->y, chipReadout->width,
		chipReadout->height / 2);
	// and binning
	slaveChip[0]->setBinning (binningVertical, binningHorizontal);
	slaveChip[1]->setBinning (binningVertical, binningHorizontal);

	ret = slaveChip[0]->startExposureInterleaved (light, exptime);
	if (ret)
		return -1;

	slave2ExposureStart.tv_usec += firstReadoutTime;

	slave2ExposureStart.tv_sec += slave2ExposureStart.tv_usec / USEC_SEC;
	slave2ExposureStart.tv_usec %= USEC_SEC;

	slaveState = SLAVE1_EXPOSING;

	chip1_light = light;
	chip1_exptime = exptime;

	return 0;
}

long
CameraMiniccdInterleavedChip::isExposing ()
{
	struct timeval now;
	long ret;

	switch (slaveState)
	{
		case SLAVE1_EXPOSING:
			gettimeofday (&now, NULL);
			if (timercmp (&now, &slave2ExposureStart, <))
				return (now.tv_sec - slave2ExposureStart.tv_sec) * USEC_SEC +
					(now.tv_usec - slave2ExposureStart.tv_usec);
			ret =
				slaveChip[1]->startExposureInterleaved (chip1_light, chip1_exptime,
				CCD_EXP_FLAGS_NOWIPE_FRAME);
			if (ret)
			{
				return -2;
			}
			slaveState = SLAVE2_EXPOSING;
		case SLAVE2_EXPOSING:
			ret = slaveChip[0]->isExposing ();
			if (ret == 2)
			{
				gettimeofday (&slave1ReadoutStart, NULL);
				ret = 1;
			}
			if (ret)
				return ret;
			// calculate firstReadoutTime
			gettimeofday (&now, NULL);
			firstReadoutTime = (now.tv_sec - slave1ReadoutStart.tv_sec) * USEC_SEC +
				(now.tv_usec - slave1ReadoutStart.tv_usec);
			slaveState = SLAVE1_READOUT;
		case SLAVE1_READOUT:
			ret = slaveChip[1]->isExposing ();
			if (ret)
				return ret;
			slaveState = SLAVE2_READOUT;
			break;
		default:
			return CameraChip::isExposing ();
	}
	return 0;
}

int
CameraMiniccdInterleavedChip::stopExposure ()
{
	return slaveChip[0]->stopExposure () | slaveChip[1]->stopExposure ();
}

int
CameraMiniccdInterleavedChip::startReadout (Rts2DevConnData * dataConn,
Rts2Conn * conn)
{
	slaveChip[0]->setReadoutConn (dataConn);
	slaveChip[1]->setReadoutConn (dataConn);
	// we use 2x2 default bin
	chipUsedReadout =
		new ChipSubset (chipReadout->x / 2, chipReadout->y / 2,
		chipReadout->getWidth () / 2,
		chipReadout->getHeight () / 2);
	usedBinningVertical = 1;
	usedBinningHorizontal = 1;
	return CameraChip::startReadout (dataConn, conn);
}

int
CameraMiniccdInterleavedChip::endReadout ()
{
	slaveChip[0]->clearReadout ();
	slaveChip[1]->clearReadout ();
	return CameraChip::endReadout ();
}

int
CameraMiniccdInterleavedChip::readoutOneLine ()
{
	int ret;
	char *row1, *row2;
	int send_data_size;

	if (sendLine == 0)
	{
		ret = CameraChip::sendFirstLine ();
		if (ret)
			return ret;
		usedRowBytes = slaveChip[0]->getUsedRowBytes () / 2;
	}

	switch (slaveState)
	{
		case SLAVE2_READOUT:
			row1 = slaveChip[0]->getAllData ();
			row2 = slaveChip[1]->getAllData ();
			if (!(row1 && row2))
			{
				// error while retriving data
				return -1;
			}
			send_top = doBinning ((uint16_t *) row1, (uint16_t *) row2);
			dest_top = send_top + ((chipUsedReadout->width / usedBinningHorizontal)
				* (chipUsedReadout->height /
				usedBinningVertical)) * 2;
			slaveState = SENDING;
		case SENDING:
			send_data_size = sendReadoutData (send_top, dest_top - send_top);
			if (send_data_size < 0)
			{
				slaveState = NO_ACTION;
				return -1;
			}
			send_top += send_data_size;
			if (send_top < dest_top)
				return 0;
		default:
			slaveState = NO_ACTION;
	}
	return -2;
} */

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
	chip_name[msg_len] = '0';
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
			!= (chipUsedSize () + CCD_MSG_IMAGE_LEN))
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
	chipUsedReadout = NULL;
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

	/*
	chipNum = msgr[CCD_CCD_FIELDS_INDEX];

	if (chipNum > 1)
	{
		// 0 chip is compostition of two interleaved exposures
		chipNum++;
		interleave = 1;
	} */

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
	/*  CCD_ELEM_TYPE msg[CCD_MSG_TEMP_LEN / CCD_ELEM_SIZE];

	  msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	  msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_TEMP_LEN;
	  msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
	  msg[CCD_MSG_INDEX] = CCD_MSG_TEMP;
	  msg[CCD_TEMP_SET_HI_INDEX] = 0;
	  msg[CCD_TEMP_SET_LO_INDEX] = 0;
	  write (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
	  read (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);*/
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
