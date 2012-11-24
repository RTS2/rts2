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

namespace rts2camd
{

/**
 * Driver for Apogee Alta cameras.
 *
 * Based on test_alta.cpp from Alta source.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Alta:public Camera
{
	public:
		Alta (int argc, char **argv);
		virtual ~ Alta (void);
		virtual int processOption (int in_opt);
		virtual int initHardware ();

		virtual int info ();

		virtual int setCoolTemp (float new_temp);
		virtual void afterNight ();

	protected:
		virtual int initChips ();
		virtual void initBinnings ();
		virtual int setBinning (int in_vert, int in_hori);
		virtual int startExposure ();
		virtual long isExposing ();
		virtual int stopExposure ();
		virtual int doReadout ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual size_t suggestBufferSize ()
		{
			return Camera::suggestBufferSize () + 4;
		}

	private:
		CApnCamera * alta;
		rts2core::ValueSelection * bitDepth;
		rts2core::ValueSelection *fanMode;

		rts2core::ValueSelection *coolerStatus;
		rts2core::ValueBool *coolerEnabled;

		rts2core::ValueInteger *cameraId;

		void setBitDepth (int newBit);
};

};

using namespace rts2camd;

void Alta::setBitDepth (int newBit)
{
	switch (newBit)
	{
		case 0:
			alta->write_DataBits (Apn_Resolution_SixteenBit);
		case 1:
			alta->write_DataBits (Apn_Resolution_TwelveBit);
	}
}

int Alta::initChips ()
{
	setSize (alta->read_RoiPixelsH (), alta->read_RoiPixelsV (), 0, 0);
	pixelX = nan ("f");
	pixelY = nan ("f");
	//  gain = alta->m_ReportedGainSixteenBit;

	return Camera::initChips ();
}

void Alta::initBinnings ()
{
	addBinning2D (1,1);
	addBinning2D (2,2);
	addBinning2D (3,3);
	addBinning2D (4,4);
	addBinning2D (8,8);
	addBinning2D (16,16);
	addBinning2D (32,32);
	addBinning2D (64,64);
	addBinning2D (3,3);
	addBinning2D (5,5);
	addBinning2D (6,6);
	addBinning2D (7,7);
	addBinning2D (10,10);
	addBinning2D (20,20);

        setBinning (1, 1);
}

int Alta::setBinning (int in_vert, int in_hori)
{
	alta->write_RoiBinningH (in_hori);
	alta->write_RoiBinningV (in_vert);
	return Camera::setBinning (in_vert, in_hori);
}

int Alta::startExposure ()
{
	int ret;

	// set region of intereset..
	alta->write_RoiStartY (chipUsedReadout->getYInt ());
	alta->write_RoiStartX (chipUsedReadout->getXInt ());
	alta->write_RoiPixelsV (chipUsedReadout->getHeightInt () / binningVertical ());
	alta->write_RoiPixelsH (chipUsedReadout->getWidthInt () / binningHorizontal ());

	ret = alta->Expose (getExposure (), getExpType () ? 0 : 1);
	if (!ret)
		return -1;

	return 0;
}

long Alta::isExposing ()
{
	long ret;
	Apn_Status status;
	ret = Camera::isExposing ();
	if (ret > 0)
		return ret;

	status = alta->read_ImagingStatus ();

	if (status < 0)
	{
		logStream (MESSAGE_ERROR) << "During check for ImagingStatus, return < 0: " << status << sendLog;
		return -2;
	}
	if (status != Apn_Status_ImageReady)
		return 200;
	// exposure has ended..
	return -2;
}

int Alta::stopExposure ()
{
	alta->StopExposure (false);
	return Camera::stopExposure ();
}

int Alta::doReadout ()
{
	int ret;

	long status;
	unsigned short width, height;
	unsigned long count;
	status = alta->GetImageData ((short unsigned int *) getDataBuffer (0), width, height, count);
	if (status != CAPNCAMERA_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "cannot read camera image " << status << ", reseting CCD" << sendLog;
		ret = alta->ResetSystem ();
		logStream (MESSAGE_ERROR) << "reset ends with " << ret << sendLog;
		if (!ret)
			return -1;
		return 0;
	}

	ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
	if (ret < 0)
		return -1;
	return -2;
}

int Alta::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == bitDepth)
	{
		setBitDepth (new_value->getValueInteger ());
		return 0;
	}
	if (old_value == coolerEnabled)
	{
		alta->write_CoolerEnable (((rts2core::ValueBool *)new_value)->getValueBool ());
		return 0;
	}
	if (old_value == fanMode)
	{
		alta->write_FanMode (new_value->getValueInteger ());
		return 0;
	}

	return Camera::setValue (old_value, new_value);
}

Alta::Alta (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
  	createValue (cameraId, "camera_id", "camera ID", false);
	cameraId->setValueInteger (1);

	createTempAir ();
	createTempCCD ();
	createTempSet ();

	createExpType ();

	createValue (coolerStatus, "COOL_STA", "cooler status", true);
	coolerStatus->addSelVal ("OFF");
	coolerStatus->addSelVal ("RAMPING_TO_SETPOINT");
	coolerStatus->addSelVal ("CORRECTING");
	coolerStatus->addSelVal ("RAMPING_TO_AMBIENT");
	coolerStatus->addSelVal ("AT_AMBIENT");
	coolerStatus->addSelVal ("AT_MAX");
	coolerStatus->addSelVal ("AT_MIN");
	coolerStatus->addSelVal ("AT_SETPOINT");

	createValue (coolerEnabled, "COOL_ENA", "if cooler is enabled", true, RTS2_VALUE_WRITABLE);

	createValue (bitDepth, "BITDEPTH", "bit depth", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	bitDepth->addSelVal ("16 bit");
	bitDepth->addSelVal ("12 bit");

	createValue (fanMode, "FAN", "fan mode", true, RTS2_VALUE_WRITABLE);
	fanMode->addSelVal ("OFF");
	fanMode->addSelVal ("LOW");
	fanMode->addSelVal ("MEDIUM");
	fanMode->addSelVal ("HIGH");

	alta = NULL;
	addOption ('b', NULL, 0, "switch to 12 bit readout mode; see alta specs for details");
	addOption ('c', NULL, 1, "night cooling temperature");
	addOption ('n', NULL, 1, "camera number (default to 1)");
}

Alta::~Alta (void)
{
	if (alta)
	{
		alta->CloseDriver ();
		delete alta;
	}
}

int Alta::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'b':
			bitDepth->setValueInteger (1);
			break;
		case 'c':
			nightCoolTemp->setValueCharArr (optarg);
			break;
		case 'n':
			cameraId->setValueCharArr (optarg);
			break;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}

int Alta::initHardware ()
{
	alta = (CApnCamera *) new CApnCamera ();
	int ret = alta->InitDriver (cameraId->getValueInteger (), 0, 0);

	if (!ret)
	{
		alta->ResetSystem ();
		alta->CloseDriver ();
		sleep (2);
		ret = alta->InitDriver (cameraId->getValueInteger (), 0, 0);
		if (!ret)
			return -1;
	}

	// Do a system reset to ensure known state, flushing enabled etc
	ret = alta->ResetSystem ();

	if (!ret)
		return -1;

	// set data bits..
	ccdRealType->setValueCharArr (alta->m_ApnSensorInfo->m_Sensor);
	serialNumber->setValueInteger (alta->m_CameraId);

	return initChips ();
}

int Alta::info ()
{
	coolerStatus->setValueInteger (alta->read_CoolerStatus ());
	coolerEnabled->setValueBool (alta->read_CoolerEnable ());
	tempSet->setValueDouble (alta->read_CoolerSetPoint ());
	tempCCD->setValueFloat (alta->read_TempCCD ());
	tempAir->setValueFloat (alta->read_TempHeatsink ());
	fanMode->setValueInteger (alta->read_FanMode ());
	return Camera::info ();
}

int Alta::setCoolTemp (float new_temp)
{
	alta->write_CoolerEnable (true);
	alta->write_FanMode (Apn_FanMode_High);
	alta->write_CoolerSetPoint (new_temp);
	return Camera::setCoolTemp (new_temp);
}

void Alta::afterNight ()
{
	alta->write_CoolerEnable (false);
	alta->write_FanMode (Apn_FanMode_Low);
	alta->write_CoolerSetPoint (100);
}

int main (int argc, char **argv)
{
	Alta device (argc, argv);
	return device.run ();
}
