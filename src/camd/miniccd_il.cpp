/* 
 * Driver for Starlight Xpress CCDs with interleaved CCD.
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


namespace rts2camd
{

class MiniccdIl:public Camera
{
	private:
		int fd_ccd;
		int fd_chip[2];
		char *row[2];

		char *device_file;

		int ccd_dac_bits;
		int sizeof_pixel;

		enum
		{
			NO_ACTION, SLAVE1_EXPOSING, SLAVE2_EXPOSING, SLAVE1_READOUT,
			SLAVE2_READOUT, SENDING
		} slaveState;
		long firstReadoutTime;
		struct timeval slave1ReadoutStart;
		struct timeval slave2ExposureStart;

		rts2core::ValueBool * tempControl;

		CCD_ELEM_TYPE msgw[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];
		CCD_ELEM_TYPE msgr[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];

		/**
		 * Start exposure on single chip.
		 */
		int startChipExposure (int chip_id, int ccd_flags = 0);
		long isChipExposing (int chip_id);

		void doBinning (uint16_t * row1, uint16_t * row2);

		void stopChipExposure (int chip_id);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int initChips ();
		virtual void initDataTypes ();

		virtual int startExposure ();
		virtual long isExposing ();
		virtual int stopExposure ();
		virtual int doReadout ();
	public:
		MiniccdIl (int argc, char **argv);
		virtual ~ MiniccdIl (void);

		virtual int setCoolTemp (float new_temp);
};

};

using namespace rts2camd;

void
MiniccdIl::doBinning (uint16_t * row1, uint16_t * row2)
{
	uint16_t *rtop = (uint16_t *) getDataBuffer (0);
	for (int r = 0; r < chipUsedReadout->getHeightInt () / 2; r++)
	{
		memcpy (rtop, row1, chipUsedReadout->getWidthInt () * sizeof (uint16_t));
		rtop += 2 * chipUsedReadout->getWidthInt ();
		row1 += chipUsedReadout->getWidthInt ();
	}
	rtop = (uint16_t *) getDataBuffer (0);
	rtop += chipUsedReadout->getWidthInt ();
	for (int r = 0; r < chipUsedReadout->getHeightInt () / 2; r++)
	{
		memcpy (rtop, row2, chipUsedReadout->getWidthInt () * sizeof (uint16_t));
		rtop += 2 * chipUsedReadout->getWidthInt ();
		row2 += chipUsedReadout->getWidthInt ();
	}

/*	#define SCALE_X   0.5
	#define SCALE_Y   0.259
	// offset aplied after scalling
	#define OFF_X   1
	#define OFF_Y   3
	// pixels which are processed ring now
	uint16_t a0_0, a0_1, a1_0, a1_1;
	uint16_t b0_0, b0_1, b1_0, b1_1;
	int w = chipUsedReadout->getWidthInt () * 2;
	int h = chipUsedReadout->getHeightInt () * 2;
	int x, y, n_x;
	long n_y;
	float n_fx, n_fy;
	int n_w, n_h;
	uint16_t *r_swap;
	// those arrays hold transformed images
	uint16_t *img_1 = new uint16_t[(w + 1) * (h + 1) * 8];
	uint16_t *img_2 = new uint16_t[(w + 1) * (h + 1) * 8];
	uint16_t *img_com = new uint16_t[(w + 1) * (h + 1) * 8];
	// split data to odd and even columns, scale by factor 0.5 and 0.259
	// init new coordinates
	n_y = 0;
	n_fy = 0;

	n_w = (int) (w / SCALE_X / 2);

	// end of image..

	// biliniear scalling
	// y is coordinate in old image, n_x and n_y are coordinates in transformed image
	for (y = 0; y < (chipUsedReadout->getHeightInt () - 1); y++)
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
	w = chipUsedReadout->getWidthInt ();
	h = chipUsedReadout->getHeightInt ();
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
			getDataBuffer (0)[y * w + x] = sum / num;
		}
		else
		{
			getDataBuffer (0)[y * w + x] = 0;
		}
	}
	delete[]img_1;
	delete[]img_2;
	delete[]img_com; */
}


int
MiniccdIl::initChips ()
{
	int in_width;
	int in_height;

	int msg_len;

	int c_len = strlen (device_file);

	for (int i = 0; i < 2; i++)
	{
		char chip_name[c_len + 2];
		strcpy (chip_name, device_file);
		chip_name[c_len] = '1' + i;
		chip_name[c_len + 1] = '\0';
	
		fd_chip[i] = open (chip_name, O_RDWR);
		if (fd_chip[i] < 0)
		{
			logStream (MESSAGE_ERROR)
				<< "MiniccdIl::initChips cannot open device file " << chip_name << ": " <<
				strerror (errno) << sendLog;
			return -1;
		}
	
		msgw[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
		msgw[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_QUERY_LEN;
		msgw[CCD_MSG_LENGTH_HI_INDEX] = 0;
		msgw[CCD_MSG_INDEX] = CCD_MSG_QUERY;
	
		write (fd_chip[i], (char *) msgw, CCD_MSG_QUERY_LEN);
		msg_len = read (fd_chip[i], (char *) msgr, CCD_MSG_CCD_LEN);
	
		if (msg_len != CCD_MSG_CCD_LEN)
		{
			logStream (MESSAGE_ERROR)
				<< "MiniccdIl::init CCD message length wrong: "
				<< msg_len << " " << strerror (errno) << sendLog;
			return -1;
		}

		if (i == 0)
		{
			in_width = msgr[CCD_CCD_WIDTH_INDEX];
			in_height = msgr[CCD_CCD_HEIGHT_INDEX];
		}
		else
		{
			if (in_width != msgr[CCD_CCD_WIDTH_INDEX])
			{
				logStream (MESSAGE_ERROR) << "Invalid " << i << " chip width." << sendLog;
				return -1;
			}
			if (in_height != msgr[CCD_CCD_HEIGHT_INDEX])
			{
				logStream (MESSAGE_ERROR) << "Invalid " << i << " chip height." << sendLog;
				return -1;
			}
		}
	}

	sizeof_pixel = (msgr[CCD_CCD_DEPTH_INDEX] + 7) / 8;

	setSize (in_width, in_height * 2, 0, 0);

	ccd_dac_bits = msgr[CCD_CCD_DAC_INDEX];

	return 0;
}


void
MiniccdIl::initDataTypes ()
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
MiniccdIl::startChipExposure (int chip_id, int ccd_flags)
{
	CCD_ELEM_TYPE msg[CCD_MSG_EXP_LEN / CCD_ELEM_SIZE];
	int exposure_msec = (int) (getExposure () * 1000);
	int ccdFlags = (getExpType () ? CCD_EXP_FLAGS_NOOPEN_SHUTTER : 0) | ccd_flags;
	int ret;

	msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_EXP_LEN;
	msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msg[CCD_MSG_INDEX] = CCD_MSG_EXP;
	msg[CCD_EXP_WIDTH_INDEX] = getUsedWidth ();
	msg[CCD_EXP_HEIGHT_INDEX] = getUsedHeight () / 2;
	msg[CCD_EXP_XOFFSET_INDEX] = chipTopX ();
	msg[CCD_EXP_YOFFSET_INDEX] = chipTopY ();
	msg[CCD_EXP_XBIN_INDEX] = binningHorizontal ();
	msg[CCD_EXP_YBIN_INDEX] = binningVertical ();
	msg[CCD_EXP_DAC_INDEX] = ccd_dac_bits;
	msg[CCD_EXP_FLAGS_INDEX] = ccdFlags;
	msg[CCD_EXP_MSEC_LO_INDEX] = exposure_msec & 0xFFFF;
	msg[CCD_EXP_MSEC_HI_INDEX] = exposure_msec >> 16;
	ret = write (fd_chip[chip_id], (char *) msg, CCD_MSG_EXP_LEN);
	if (ret != CCD_MSG_EXP_LEN)
	{
		return -1;
	}
	logStream (MESSAGE_DEBUG) << "start chip exposing " << chip_id << sendLog;
	return 0;
}

int MiniccdIl::startExposure ()
{
	int ret;

	gettimeofday (&slave2ExposureStart, NULL);

	ret = startChipExposure (0);
	if (ret)
		return ret;

	slave2ExposureStart.tv_usec += firstReadoutTime;

	slave2ExposureStart.tv_sec += slave2ExposureStart.tv_usec / USEC_SEC;
	slave2ExposureStart.tv_usec %= USEC_SEC;

	slaveState = SLAVE1_EXPOSING;

	return 0;
}

long MiniccdIl::isChipExposing (int chip_id)
{
  	fd_set set;
	struct timeval read_tout;
	ssize_t ret;

	FD_ZERO (&set);
	FD_SET (fd_chip[chip_id], &set);
	read_tout.tv_sec = read_tout.tv_usec = 0;
	select (fd_chip[chip_id] + 1, &set, NULL, NULL, &read_tout);
	if (FD_ISSET (fd_chip[chip_id], &set))
	{
		CCD_ELEM_TYPE msgi[CCD_MSG_IMAGE_LEN];
		ret = read (fd_chip[chip_id], msgi, CCD_MSG_IMAGE_LEN);
		if (ret != CCD_MSG_IMAGE_LEN)
		{
			logStream (MESSAGE_DEBUG) << "ret != CCD_MSG_INDEX " << ret << sendLog;
			return -1;
		}
		// test image header
		if (msgi[CCD_MSG_INDEX] != CCD_MSG_IMAGE)
		{
			logStream (MESSAGE_ERROR) <<
				"miniccd sendLineData wrong image message" << sendLog;
			return -1;
		}
		if ((unsigned int) (msgi[CCD_MSG_LENGTH_LO_INDEX] + (msgi[CCD_MSG_LENGTH_HI_INDEX] << 16))
			!= (chipByteSize () / 2 + CCD_MSG_IMAGE_LEN))
		{
			logStream (MESSAGE_ERROR) << "miniccd sendLineData wrong size " <<
				msgi[CCD_MSG_LENGTH_LO_INDEX] +
				(msgi[CCD_MSG_LENGTH_HI_INDEX] << 16) << sendLog;
			return -1;
		}
		delete[] row[chip_id];
		row[chip_id] = new char[chipByteSize () / 2];

		int rlen = 0;

		if (chip_id == 0)
			gettimeofday (&slave1ReadoutStart, NULL);

		while (rlen < (chipByteSize () / 2))
		{
			std::cout << "chip " << chip_id << " rlen " << rlen << std::endl;
			ret = read (fd_chip[chip_id], row[chip_id] + rlen, (chipByteSize () / 2) - rlen);
			if (ret == -1)
			{
				if (errno != EINTR)
				{
					logStream (MESSAGE_DEBUG)
						<< "chip_id " << chip_id
						<< "return != chipByteSize " << ret << " " << chipByteSize () << sendLog;
					return -1;
				}
			}
			else
			{
				rlen += ret;
			}
		}
		std::cout << "chip " << chip_id << " rlen " << rlen << std::endl;
		return -2;
	}
	return 100;
}

long MiniccdIl::isExposing ()
{
	struct timeval now;
	long ret;

	switch (slaveState)
	{
		case SLAVE1_EXPOSING:
			gettimeofday (&now, NULL);
			if (timercmp (&now, &slave2ExposureStart, <))
			{
				timersub (&slave2ExposureStart, &now, &now);
				return now.tv_sec * USEC_SEC + now.tv_usec;
			}
			ret = startChipExposure (1, CCD_EXP_FLAGS_NOWIPE_FRAME);
			if (ret)
			{
				return -2;
			}
			slaveState = SLAVE2_EXPOSING;
		case SLAVE2_EXPOSING:
			ret = isChipExposing (0);
			if (ret != -2)
				return ret;
			// calculate firstReadoutTime
			gettimeofday (&now, NULL);
			timersub (&now, &slave1ReadoutStart, &slave1ReadoutStart);
			firstReadoutTime = slave1ReadoutStart.tv_sec * USEC_SEC + slave1ReadoutStart.tv_usec;
			slaveState = SLAVE1_READOUT;
		case SLAVE1_READOUT:
			ret = isChipExposing (1);
			if (ret)
				return ret;
			slaveState = SLAVE2_READOUT;
			break;
		default:
			return -1;
	}
	return -2;
}

int MiniccdIl::doReadout ()
{
	int ret;

//		usedRowBytes = slaveChip[0]->getUsedRowBytes () / 2;

	switch (slaveState)
	{
		case SLAVE2_READOUT:
			doBinning ((uint16_t *) (row[0]), (uint16_t *) (row[1]));
			slaveState = SENDING;
		case SENDING:
			ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
			if (ret < 0)
			{
				slaveState = NO_ACTION;
				return -1;
			}
			if (getWriteBinaryDataSize () == 0)
				return -2;
			return 0;
		default:
			slaveState = NO_ACTION;
	}
	return -2;
}

void MiniccdIl::stopChipExposure (int chip_id)
{
	CCD_ELEM_TYPE msg[CCD_MSG_ABORT_LEN / CCD_ELEM_SIZE];
	/*
	 * Send the abort request.
	 */
	msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_ABORT_LEN;
	msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msg[CCD_MSG_INDEX] = CCD_MSG_ABORT;
	write (fd_chip[chip_id], (char *) msg, CCD_MSG_ABORT_LEN);
}

int MiniccdIl::stopExposure ()
{
	stopChipExposure (0);
	stopChipExposure (1);
	return Camera::stopExposure ();
}

MiniccdIl::MiniccdIl (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	fd_ccd = -1;
	device_file = NULL;

	row[0] = NULL;
	row[1] = NULL;

	firstReadoutTime = 3 * USEC_SEC;
	slaveState = NO_ACTION;

	createExpType ();

	addOption ('f', "device_file", 1, "miniccd device file (/dev/xxx entry)");
}


MiniccdIl::~MiniccdIl (void)
{
	close (fd_ccd);
	close (fd_chip[0]);
	close (fd_chip[1]);
}


int
MiniccdIl::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}


int
MiniccdIl::init ()
{
	int ret;
	int msg_len;

	ret = Camera::init ();
	if (ret)
		return ret;

	createValue (tempControl, "temp_control", "temperature control", false);
	tempControl->setValueBool (false);

	fd_ccd = open (device_file, O_RDWR);
	if (fd_ccd < 0)
	{
		logStream (MESSAGE_ERROR)
			<< "MiniccdIl::init Unable to open device: "
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

	int chipNum = msgr[CCD_CCD_FIELDS_INDEX];

	if (chipNum == 1)
	{
		logStream (MESSAGE_ERROR) << "system with a single chip cannot be interleaved. Plese try non-interleaved driver." << sendLog;
		return -1;
	}

	ccdRealType->setValueCharArr ((char *) &msgr[CCD_CCD_NAME_INDEX]);

	return initChips ();
}

int MiniccdIl::setCoolTemp (float _temp)
{
	if (!tempControl->getValueBool ())
		return 0;

	CCD_ELEM_TYPE msg[CCD_MSG_TEMP_LEN / CCD_ELEM_SIZE];

	msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
	msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_TEMP_LEN;
	msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
	msg[CCD_MSG_INDEX] = CCD_MSG_TEMP;
	msg[CCD_TEMP_SET_HI_INDEX] = 0;
	msg[CCD_TEMP_SET_LO_INDEX] = 0;
	write (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
	read (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
	return Camera::setCoolTemp (_temp);
}

int
main (int argc, char **argv)
{
	MiniccdIl device (argc, argv);
	return device.run ();
}
