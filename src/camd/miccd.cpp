/*
 * Driver for Moravian Instruments (Moravske pristroje) CCDs.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "camd.h"
#include "miccd.h"

namespace rts2camd
{
/**
 * Driver for MI CCD.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MICCD:public Camera
{
	public:
		MICCD (int argc, char **argv);
		virtual ~MICCD ();
	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int initValues ();

		virtual void initBinnings ()
		{
			Camera::initBinnings ();

			addBinning2D (2, 2);
			addBinning2D (3, 3);
			addBinning2D (4, 4);
			addBinning2D (2, 1);
			addBinning2D (3, 1);
			addBinning2D (4, 1);
			addBinning2D (1, 2);
			addBinning2D (1, 3);
			addBinning2D (1, 4);
		}

		virtual int info ();

		virtual int setBinning (int in_vert, int in_hori);
		virtual int setCoolTemp (float new_temp);

		virtual int startExposure ();
		virtual int endExposure ();

		virtual int doReadout ();
	private:
		Rts2ValueLong *id;
		int fd;
		camera_info_t cami;
};

}

using namespace rts2camd;

MICCD::MICCD (int argc, char **argv):Camera (argc, argv)
{
	fd = -1;

	createTempAir ();
	createTempCCD ();
	createTempSet ();

	createValue (id, "product_id", "camera product identification", false);


	addOption ('p', NULL, 1, "MI CCD product ID");
}

MICCD::~MICCD ()
{
	if (fd >= 0)
		miccd_close (fd);
}

int MICCD::processOption (int opt)
{
	switch (opt)
	{
		case 'p':
			id->setValueCharArr (optarg);
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int MICCD::init ()
{
	int ret;
	ret = Camera::init ();
	if (ret)
		return ret;
	fd = miccd_open (id->getValueLong ());
	if (fd < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot find device with id " << id->getValueLong () << sendLog;
		return -1;
	}
	return 0;
}

int MICCD::initValues ()
{
	int ret;
	ret = miccd_info (fd, &cami);
	if (ret)
		return -1;

	addConstValue ("DESCRIPTION", "camera descriptio", cami.description);
	addConstValue ("SERIAL", "camera serial number", cami.serial);
	addConstValue ("CHIP", "camera chip", cami.chip);

	initCameraChip (cami.w, cami.h, cami.pw, cami.ph);

	return Camera::initValues ();
}

int MICCD::info ()
{
	int ret;
	uint16_t val;
	ret = miccd_chip_temperature (fd, &val);
	if (ret)
		return -1;
	tempCCD->setValueDouble (val / 10.0);
	ret = miccd_environment_temperature (fd, &val);
	if (ret)
		return -1;
	tempAir->setValueDouble (val / 10.0);
	return Camera::info ();
}

int MICCD::setBinning (int in_vert, int in_hori)
{
	int ret;
	ret = miccd_binning (fd, in_hori, in_vert);
	if (ret)
		return -1;
	return Camera::setBinning (in_vert, in_hori);
}

int MICCD::setCoolTemp (float new_temp)
{
	int ret;
	ret = miccd_set_cooltemp (fd, new_temp * 10);
	if (ret)
		return -1;
	return 0;
}

int MICCD::startExposure ()
{
	int ret;
	
	ret = miccd_clear (fd);
	if (ret)
		return -1;
	ret = miccd_hclear (fd);
	if (ret)
		return -1;
	ret = miccd_open_shutter (fd);
	if (ret)
		return -1;
	return 0;
}

int MICCD::endExposure ()
{
	int ret;
	ret = miccd_close_shutter (fd);
	if (ret)
		return ret;
	ret = miccd_shift_to0 (fd);
	if (ret)
		return ret;
	return Camera::endExposure ();
}

int MICCD::doReadout ()
{
	int ret;
	char *bufferTop = dataBuffer;
	size_t s = 0xffff;

	while (bufferTop < dataBuffer + dataBufferSize)
	{
		if (bufferTop + s > dataBuffer + dataBufferSize)
			s = dataBuffer + dataBufferSize - bufferTop;
		ret = miccd_data (fd, bufferTop, s);
		if (ret)
			return -1;
	}

	ret = sendReadoutData (dataBuffer, getWriteBinaryDataSize ());
	if (ret < 0)
		return ret;
	return -2;
}

int main (int argc, char **argv)
{
	MICCD device (argc, argv);
	return device.run ();
}
