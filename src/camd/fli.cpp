/*
 * Driver for FLI CCDs.
 * Copyright (C) 2005-2012 Petr Kubanek <petr@kubanek.net>
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

/**
 * @file fli.cpp FLI camera driver
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * You will need FLIlib and option to ./configure (--with-fli=<llibflidir>)
 * to get that running. You can get libfli on http://www.moronski.com/fli
 */

#include "camd.h"

#include "libfli.h"

#define FLIUSB_CAM_ID      0x02
#define FLIUSB_PROLINE_ID  0x0a

namespace rts2camd
{

/**
 * Class for control of FLI CCD.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Fli:public Camera
{
	public:
		Fli (int in_argc, char **in_argv);
		virtual ~ Fli (void);

		virtual int processOption (int in_opt);

		virtual int initHardware ();

		virtual int setCoolTemp (float new_temp);
		virtual void afterNight ();
	protected:
		virtual int initChips ();

		virtual void initBinnings ()
		{
			Camera::initBinnings ();

			addBinning2D (2, 2);
			addBinning2D (4, 4);
			addBinning2D (8, 8);
			addBinning2D (16, 16);
			addBinning2D (2, 1);
			addBinning2D (4, 1);
			addBinning2D (8, 1);
			addBinning2D (16, 1);
			addBinning2D (1, 2);
			addBinning2D (1, 4);
			addBinning2D (1, 8);
			addBinning2D (1, 16);
		}

		virtual int info ();
		
		virtual int switchCooling (bool cooling);

		virtual void temperatureCheck ();

		virtual int startExposure ();
		virtual long isExposing ();
		virtual int stopExposure ();
		virtual int doReadout ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

	private:
		rts2core::ValueFloat *coolPower;
		rts2core::ValueSelection *fliShutter;
		rts2core::ValueBool *useBgFlush;
		rts2core::ValueSelection *fliMode;

		int bgFlush;

		flidomain_t deviceDomain;

		flidev_t dev;

		long hwRev;
		int camNum;
		char *camName;

		int fliDebug;
		rts2core::ValueInteger *nflush;
};

};

using namespace rts2camd;

int Fli::initChips ()
{
	LIBFLIAPI ret;
	long x, y, w, h;

	ret = FLIGetVisibleArea (dev, &x, &y, &w, &h);
	if (ret)
		return -1;
	// put true width & height
	w -= x;
	h -= y;
	setSize ((int) w, (int) h, (int) x, (int) y);

	ret = FLIGetPixelSize (dev, &pixelX, &pixelY);
	if (ret)
		return -1;

	return 0;
}

int Fli::startExposure ()
{
	LIBFLIAPI ret;

	ret = FLISetVBin (dev, binningVertical ());
	if (ret)
		return -1;
	ret = FLISetHBin (dev, binningHorizontal ());
	if (ret)
		return -1;

	ret = FLISetImageArea (dev, chipTopX (), chipTopY (), chipTopX () + getUsedWidthBinned (), chipTopY () + getUsedHeightBinned ());
	if (ret)
		return -1;

	ret = FLISetExposureTime (dev, (long) (getExposure () * 1000));
	if (ret)
		return -1;

	ret = FLISetFrameType (dev, (getExpType () == 0) ? FLI_FRAME_TYPE_NORMAL : FLI_FRAME_TYPE_DARK);
	if (ret)
		return -1;

	ret = FLIExposeFrame (dev);
	if (ret)
		return -1;
	return 0;
}

long Fli::isExposing ()
{
	LIBFLIAPI ret;
	long tl;
	ret = FLIGetExposureStatus (dev, &tl);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "failed at isExposing" << sendLog;
		return -1;
	}

	if (tl <= 0)
		return -2;

	return tl * 1000;			 // we get tl in msec, needs to return usec
}

int Fli::stopExposure ()
{
	LIBFLIAPI ret;
	long timer;
	ret = FLIGetExposureStatus (dev, &timer);
	if (ret == 0 && timer > 0)
	{
		ret = FLICancelExposure (dev);
		if (ret)
			return ret;
		ret = FLIFlushRow (dev, getHeight (), 1);
		if (ret)
			return ret;
	}
	return Camera::stopExposure ();
}

int Fli::doReadout ()
{
	LIBFLIAPI ret;
	char *bufferTop = getDataBuffer (0);
	for (int line = 0; line < getUsedHeightBinned (); line++, bufferTop += usedPixelByteSize () * getUsedWidthBinned ())
	{
		ret = FLIGrabRow (dev, bufferTop, getUsedWidthBinned ());
		if (ret)
			return -1;
	}
	ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
	if (ret < 0)
	{
		return ret;
	}
	return -2;
}

int Fli::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == fliShutter)
	{
		int ret;
		switch (new_value->getValueInteger ())
		{
			case 0:
				ret = FLI_SHUTTER_CLOSE;
				break;
			case 1:
				ret = FLI_SHUTTER_OPEN;
				break;
			case 2:
				ret = FLI_SHUTTER_EXTERNAL_TRIGGER;
				break;
			case 3:
				ret = FLI_SHUTTER_EXTERNAL_TRIGGER_LOW;
				break;
			case 4:
				ret = FLI_SHUTTER_EXTERNAL_TRIGGER_HIGH;
				break;
		}
		ret = FLIControlShutter (dev, ret);
		return ret ? -2 : 0;
	}
	if (old_value == nflush)
	{
		return FLISetNFlushes (dev, new_value->getValueInteger ()) ? -2 : 0;
	}
	if (old_value == useBgFlush)
	{
		return FLIControlBackgroundFlush (dev, ((rts2core::ValueBool *) new_value)->getValueBool () ? FLI_BGFLUSH_START : FLI_BGFLUSH_STOP) ? -2 : 0;
	}
	if (old_value == fliMode)
	{
		return FLISetCameraMode (dev, new_value->getValueInteger ()) ? -2 : 0;
	}
	return Camera::setValue (old_value, new_value);
}

Fli::Fli (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	createTempAir ();
	createTempSet ();
	createTempCCD ();
	createTempCCDHistory ();
	createExpType ();

	coolPower = NULL;

	createValue (fliShutter, "FLISHUT", "FLI shutter state", true, RTS2_VALUE_WRITABLE);
	fliShutter->addSelVal ("CLOSED");
	fliShutter->addSelVal ("OPENED");
	fliShutter->addSelVal ("EXTERNAL TRIGGER");
	fliShutter->addSelVal ("EXTERNAL TRIGGER LOW");
	fliShutter->addSelVal ("EXTERNAL TRIGGER HIGH");

	bgFlush = -1;
	useBgFlush = NULL;

	deviceDomain = FLIDEVICE_CAMERA | FLIDOMAIN_USB;
	fliDebug = FLIDEBUG_NONE;
	hwRev = -1;
	camNum = 0;
	camName = NULL;

	createValue (nflush, "nflush", "number of flushes before exposure", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	nflush->setValueInteger (-1);

	addOption ('D', NULL, 1, "CCD Domain (default to USB; possible values: USB|LPT|SERIAL|INET)");
	addOption ('R', NULL, 1, "find camera by HW revision");
	addOption ('B', NULL, 1, "FLI debug level (1, 2 or 3; 3 will print most error message to stdout)");
	addOption ('n', NULL, 1, "Camera number (in FLI list)");
	addOption ('N', NULL, 1, "Camera serial number (string, device name)");
	addOption ('b', NULL, 0, "use background flush, supported by several newer CCDs (do not confuse with normal flush, initiated by parameter -l)");
	addOption ('l', NULL, 1, "Number of CCD flushes");
	addOption ('f', NULL, 1, "FLI device path");
}

Fli::~Fli (void)
{
	FLIClose (dev);
}

int Fli::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'D':
			deviceDomain = FLIDEVICE_CAMERA;
			if (!strcasecmp ("USB", optarg))
				deviceDomain |= FLIDOMAIN_USB;
			else if (!strcasecmp ("LPT", optarg))
				deviceDomain |= FLIDOMAIN_PARALLEL_PORT;
			else if (!strcasecmp ("SERIAL", optarg))
				deviceDomain |= FLIDOMAIN_SERIAL;
			else if (!strcasecmp ("INET", optarg))
				deviceDomain |= FLIDOMAIN_INET;
			else
				return -1;
			break;
		case 'R':
			hwRev = atol (optarg);
			break;
		case 'B':
			switch (atoi (optarg))
			{
				case 1:
					fliDebug = FLIDEBUG_FAIL;
					break;
				case 2:
					fliDebug = FLIDEBUG_FAIL | FLIDEBUG_WARN;
					break;
				case 3:
					fliDebug = FLIDEBUG_ALL;
					break;
				default:
					return -1;
			}
			break;
		case 'n':
			camNum = atoi (optarg);
			break;
		case 'N':
			camName = optarg;
			break;
		case 'l':
			nflush->setValueCharArr (optarg);
			break;
		case 'b':
			bgFlush = 1;
			break;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}

int Fli::initHardware ()
{
	LIBFLIAPI ret;

	char **names;
	char *nam_sep;
	char **nam;					 // current name

	const char *devnam;

	if (fliDebug)
		FLISetDebugLevel (NULL, fliDebug);

	ret = FLIList (deviceDomain, &names);
	if (ret)
		return -1;

	if (names[0] == NULL)
	{
		logStream (MESSAGE_ERROR) << "Fli::init No device found!" << sendLog;
		return -1;
	}

	// find device based on serial number..
	if (camName != NULL)
	{
		nam = names;
		while (*nam)
		{
			// separate semicolon
			nam_sep = strchr (*nam, ';');
			if (nam_sep)
				*nam_sep = '\0';
			ret = FLIOpen (&dev, *nam, deviceDomain);
			if (ret)
			{
				nam++;
				continue;
			}
			ret = FLIGetDeviceName (dev, &devnam);
			if (!ret && !strcmp (devnam, camName))
			{
				break;
			}
			// skip to next camera
			FLIClose (dev);
			nam++;
		}
	}
	// find device based on hw revision..
	else if (hwRev > 0)
	{
		nam = names;
		while (*nam)
		{
			// separate semicolon
			long cam_hwrev;
			nam_sep = strchr (*nam, ';');
			if (nam_sep)
				*nam_sep = '\0';
			ret = FLIOpen (&dev, *nam, deviceDomain);
			if (ret)
			{
				nam++;
				continue;
			}
			ret = FLIGetHWRevision (dev, &cam_hwrev);
			if (!ret && cam_hwrev == hwRev)
			{
				break;
			}
			// skip to next camera
			FLIClose (dev);
			nam++;
		}
	}
	// find camera by camera number
	else if (camNum > 0)
	{
		nam = names;
		int i = 1;
		while (*nam && i != camNum)
		{
			nam++;
			i++;
		}
		if (!*nam)
			return -1;
		i--;
		nam_sep = strchr (names[i], ';');
		*nam_sep = '\0';
		ret = FLIOpen (&dev, names[i], deviceDomain);
	}
	else
	{
		nam_sep = strchr (names[0], ';');
		if (nam_sep)
			*nam_sep = '\0';
		ret = FLIOpen (&dev, names[0], deviceDomain);
	}

	FLIFreeList (names);

	if (ret)
		return -1;

	if (nflush->getValueInteger () >= 0)
	{
		ret = FLISetNFlushes (dev, nflush->getValueInteger ());
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "fli init FLISetNFlushes ret " << ret << sendLog;
			return -1;
		}
		logStream (MESSAGE_DEBUG) << "fli init set Nflush to " << nflush->getValueInteger () <<	sendLog;
	}

	FLIGetDeviceName (dev, &devnam);
	serialNumber->setValueCharArr (devnam);

	long hwrev;
	long fwrev;
	char ccdType[64];
	ret = FLIGetModel (dev, ccdType, 64);
	if (ret)
		return -1;
	ret = FLIGetHWRevision (dev, &hwrev);
	if (ret)
		return -1;
	ret = FLIGetFWRevision (dev, &fwrev);
	if (ret)
		return -1;
	sprintf (ccdType, "FLI %li.%li", hwrev, fwrev);
	ccdRealType->setValueCharArr (ccdType);

	long devid;
	ret = FLIGetDeviceID (dev, &devid);
	if (ret)
	  	return -1;

	if ((devid == FLIUSB_PROLINE_ID && fwrev > 0x0100) || bgFlush != -1)
	{
		createValue (useBgFlush, "BGFLUSH", "use BG flush", true, RTS2_VALUE_WRITABLE);
		useBgFlush->setValueBool (bgFlush == 1);
		ret = FLIControlBackgroundFlush (dev, useBgFlush->getValueBool () ? FLI_BGFLUSH_START : FLI_BGFLUSH_STOP);
		if (ret)
			return -1;
	}

	double cp;
	ret = FLIGetCoolerPower (dev, &cp);
	if (!ret)
	{
		createValue (coolPower, "COOL_PWR", "cooling power", true);
	}

	// get mode..
	createValue (fliMode, "RDOUTM", "readout mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	// discover available modes..
	flimode_t m = 0;
	char mode[50];
	while ((ret = FLIGetCameraModeString (dev, m, mode, 50)) == 0)
	{
		fliMode->addSelVal (mode);
		m++;
	}

	return initChips ();
}

int Fli::info ()
{
  	if (!isIdle ())
		return Camera::info ();
	LIBFLIAPI ret;
	double fliTemp;
	ret = FLIReadTemperature (dev, FLI_TEMPERATURE_EXTERNAL, &fliTemp);
	if (ret)
		return -1;
	tempAir->setValueFloat (fliTemp);
	ret = FLIReadTemperature (dev, FLI_TEMPERATURE_INTERNAL, &fliTemp);
	if (ret)
		return -1;
	tempCCD->setValueFloat (fliTemp);
	if (coolPower)
	{
		ret = FLIGetCoolerPower (dev, &fliTemp);
		if (ret)
			return -1;
		coolPower->setValueFloat (fliTemp);
	}
	return Camera::info ();
}

int Fli::switchCooling (bool cooling)
{
	if (cooling)
	{
		return FLISetTemperature (dev, tempSet->getValueFloat ());
	}
	else
	{
		return FLISetTemperature (dev, 100) ? -1:0;
	}
}

void Fli::temperatureCheck ()
{
	if (!isIdle ())
		return;

	LIBFLIAPI ret;
	double fliTemp;
	ret = FLIGetTemperature (dev, &fliTemp);
	if (ret)
		return;
	addTempCCDHistory (fliTemp);
}

int Fli::setCoolTemp (float new_temp)
{
	LIBFLIAPI ret;
	ret = FLISetTemperature (dev, new_temp);
	if (ret)
		return -1;
	return Camera::setCoolTemp (new_temp);
}

void Fli::afterNight ()
{
	setCoolTemp (40);
	Camera::afterNight ();
}

//int Fli::idle ()
//{
// we want to know the current noise
// charactristic of the chip. So we read out a sample of 256 pixels and compute
// the noise&level
//}

int main (int argc, char **argv)
{
	Fli device (argc, argv);
	return device.run ();
}
