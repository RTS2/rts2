/* 
 * Apogee Alta CCD driver.
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

#include <math.h>

#include "camd.h"
#include "ApnCamera.h"

/**
 * Driver for Apogee Alta cameras.
 *
 * Based on test_alta.cpp from Alta source.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevCameraAlta:public Rts2DevCamera
{
	private:
		CApnCamera * alta;
		int bit12;

	protected:
		virtual int initChips ();
		virtual int setBinning (int in_vert, int in_hori);
		virtual int startExposure ();
		virtual long isExposing ();
		virtual int stopExposure ();
		virtual int readoutOneLine ();

	public:
		Rts2DevCameraAlta (int argc, char **argv);
		virtual ~ Rts2DevCameraAlta (void);
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int ready ();
		virtual int info ();

		virtual int camChipInfo (int chip);

		virtual int camCoolMax ();
		virtual int camCoolHold ();
		virtual int camCoolTemp (float new_temp);
		virtual int camCoolShutdown ();
};

int
Rts2DevCameraAlta::initChips ()
{
	setSize (alta->read_RoiPixelsH (), alta->read_RoiPixelsV (), 0, 0);
	setBinning (1, 1);
	pixelX = nan ("f");
	pixelY = nan ("f");
	//  gain = alta->m_ReportedGainSixteenBit;

	return Rts2DevCamera::initChips ();
}


int
Rts2DevCameraAlta::setBinning (int in_vert, int in_hori)
{
	alta->write_RoiBinningH (in_hori);
	alta->write_RoiBinningV (in_vert);
	return Rts2DevCamera::setBinning (in_vert, in_hori);
}


int
Rts2DevCameraAlta::startExposure ()
{
	int ret;
	ret = alta->Expose (getExposure (), getExpType () ? 0 : 1);
	if (!ret)
		return -1;

	// set region of intereset..
	alta->write_RoiStartY (chipUsedReadout->getYInt ());
	alta->write_RoiStartX (chipUsedReadout->getXInt ());
	alta->write_RoiPixelsH (chipUsedReadout->getWidthInt ());
	alta->write_RoiPixelsV (chipUsedReadout->getHeightInt ());

	return 0;
}


long
Rts2DevCameraAlta::isExposing ()
{
	long ret;
	Apn_Status status;
	ret = Rts2DevCamera::isExposing ();
	if (ret > 0)
		return ret;

	status = alta->read_ImagingStatus ();

	if (status < 0)
		return -2;
	if (status != Apn_Status_ImageReady)
		return 200;
	// exposure has ended..
	return -2;
}


int
Rts2DevCameraAlta::stopExposure ()
{
	// we need to digitize image:(
	alta->StopExposure (true);
	return Rts2DevCamera::stopExposure ();
}


int
Rts2DevCameraAlta::readoutOneLine ()
{
	int ret;

	bool status;
	unsigned long count;
	unsigned short width = chipUsedReadout->getWidthInt ();
	unsigned short height = chipUsedReadout->getHeightInt ();
	status = alta->GetImageData ((short unsigned int *) dataBuffer, width, height, count);
	if (!status)
		return -1;

	ret = sendReadoutData (dataBuffer, dataBufferSize);
	if (ret < 0)
		return -1;
	return -2;
}


Rts2DevCameraAlta::Rts2DevCameraAlta (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
	createTempAir ();
	createTempCCD ();
	createTempRegulation ();
	createTempSet ();
	createCamFan ();

	alta = NULL;
	addOption ('B', "12bits", 0,
		"switch to 12 bit readout mode; see alta specs for details");
	bit12 = 0;
}


Rts2DevCameraAlta::~Rts2DevCameraAlta (void)
{
	if (alta)
	{
		alta->CloseDriver ();
		delete alta;
	}
}


int
Rts2DevCameraAlta::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'B':
			bit12 = 1;
			break;
		default:
			return Rts2DevCamera::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevCameraAlta::init ()
{
	int ret;
	ret = Rts2DevCamera::init ();
	if (ret)
	{
		return ret;
	}
	alta = (CApnCamera *) new CApnCamera ();
	ret = alta->InitDriver (0, 0, 0);

	if (!ret)
	{
		alta->ResetSystem ();
		alta->CloseDriver ();
		sleep (2);
		ret = alta->InitDriver (0, 0, 0);
		if (!ret)
			return -1;
	}

	// Do a system reset to ensure known state, flushing enabled etc
	ret = alta->ResetSystem ();

	if (!ret)
		return -1;

	// set data bits..
	if (bit12)
	{
		alta->write_DataBits (Apn_Resolution_TwelveBit);
	}
	else
	{
		alta->write_DataBits (Apn_Resolution_SixteenBit);
	}

	strcpy (ccdType, "Alta ");
	strncat (ccdType, alta->m_ApnSensorInfo->m_Sensor, 10);
	sprintf (serialNumber, "%i", alta->m_CameraId);

	return initChips ();
}


int
Rts2DevCameraAlta::ready ()
{
	int ret;
	ret = alta->read_Present ();
	return (ret ? 0 : -1);
}


int
Rts2DevCameraAlta::info ()
{
	tempRegulation->setValueInteger (alta->read_CoolerEnable ());
	tempSet->setValueFloat (alta->read_CoolerSetPoint ());
	tempCCD->setValueFloat (alta->read_TempCCD ());
	tempAir->setValueFloat (alta->read_TempHeatsink ());
	fan->setValueInteger (alta->read_FanMode () == Apn_FanMode_Low ? 0 : 1);
	return Rts2DevCamera::info ();
}


int
Rts2DevCameraAlta::camChipInfo (int chip)
{
	return 0;
}


int
Rts2DevCameraAlta::camCoolMax ()
{
	alta->write_CoolerEnable (true);
	alta->write_FanMode (Apn_FanMode_High);
	alta->write_CoolerSetPoint (-60);
	return 0;
}


int
Rts2DevCameraAlta::camCoolHold ()
{
	alta->write_CoolerEnable (true);
	alta->write_FanMode (Apn_FanMode_High);
	return 0;
}


int
Rts2DevCameraAlta::camCoolTemp (float new_temp)
{
	alta->write_CoolerEnable (true);
	alta->write_FanMode (Apn_FanMode_High);
	alta->write_CoolerSetPoint (new_temp);
	return 0;
}


int
Rts2DevCameraAlta::camCoolShutdown ()
{
	alta->write_CoolerEnable (false);
	alta->write_FanMode (Apn_FanMode_Low);
	alta->write_CoolerSetPoint (100);
	return 0;
}


int
main (int argc, char **argv)
{
	Rts2DevCameraAlta device = Rts2DevCameraAlta (argc, argv);
	return device.run ();
}
