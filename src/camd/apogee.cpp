/* 
 * Apogee CCD driver.
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
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define BOOLEAN  bool
#define HANDLE   int

#define MAXTOTALROWS     5000
#define MAXTOTALCOLUMNS  5000

#include "apogee/CameraIO_Linux.h"

#include "camd.h"

// Error codes returned from config_load
const int CCD_OPEN_NOERR = 0;	 // No error detected
const int CCD_OPEN_CFGNAME = 1;	 // No config file specified
const int CCD_OPEN_CFGDATA = 2;	 // Config missing or missing required data
const int CCD_OPEN_LOOPTST = 3;	 // Loopback test failed, no camera found
const int CCD_OPEN_ALLOC = 4;	 // Memory alloc failed - system error
const int CCD_OPEN_NTIO = 5;	 // NT I/O driver not present

namespace rts2camd
{

/**
 * Driver for Apogee camera.
 *
 * Based on original Apogee driver.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Apogee:public Camera
{
	public:
		Apogee (int argc, char **argv);
		virtual ~ Apogee (void);
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();

		virtual int setCoolTemp (float new_temp);
		virtual void afterNight ();

	protected:
		virtual int initChips ();
		virtual void initBinnings ();
		virtual int setBinning (int in_vert, int in_hori);
		virtual int startExposure ();
		virtual long isExposing ();
		virtual int endExposure (int ret);
		virtual int stopExposure ();
		virtual int doReadout ();
		virtual int endReadout ();

	private:
		CCameraIO *camera;
		int device_id;
		const char *cfgname;
		int config_load (short BaseAddress, short RegOffset);
		bool CfgGet (FILE * inifp, const char *inisect, const char *iniparm, char *retbuff,
			short bufflen, short *parmlen);

		time_t expExposureEnd;

		rts2core::ValueSelection *coolerStatus;
		rts2core::ValueSelection *coolerMode;
};

};

using namespace rts2camd;

int Apogee::initChips ()
{
	setSize (camera->m_ImgColumns, camera->m_ImgRows, 0, 0);
	pixelX = camera->m_PixelXSize;
	pixelY = camera->m_PixelYSize;

	return Camera::initChips ();
}

void Apogee::initBinnings ()
{
	Camera::initBinnings ();
	setBinning (1, 1);
}

int Apogee::setBinning (int in_vert, int in_hori)
{
	camera->m_ExposureBinX = in_hori;
	camera->m_ExposureBinY = in_vert;
	return Camera::setBinning (in_vert, in_hori);
}

int Apogee::startExposure ()
{
	bool ret;
	ret = camera->Expose (getExposure (), getExpType () ? 0 : 1);

	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "apogee startExposure exposure error" << sendLog;
		return -1;
	}

	expExposureEnd = 0;
	return 0;
}

long Apogee::isExposing ()
{
	long ret;
	time_t now;
	Camera_Status status;
	ret = Camera::isExposing ();
	if (ret > 0)
		return ret;

	if (expExposureEnd == 0)
	{
		time (&expExposureEnd);
		expExposureEnd += 10;
	}

	status = camera->read_Status ();

	if (status != Camera_Status_ImageReady)
	{
		if (status == Camera_Status_Flushing
			|| status == Camera_Status_Exposing)
		{
			// if flushing takes too much time, reset camera and end exposure
			time (&now);
			if (expExposureEnd < now)
			{
				camera->Reset ();
				camera->Flush ();
				return -1;
			}
		}
		return 200;
	}

	// exposure has ended..
	return -2;
}

int Apogee::endExposure (int ret)
{
	camera->m_WaitingforTrigger = false;
	return Camera::endExposure (ret);
}

int Apogee::stopExposure ()
{
	camera->Reset ();
	camera->Flush ();
	return Camera::stopExposure ();
}

int Apogee::doReadout ()
{
	bool status;
	short int width, height;
	width = chipUsedReadout->getWidthInt ();
	height = chipUsedReadout->getHeightInt ();
	int ret;

	camera->m_ExposureStartX = chipUsedReadout->getXInt ();
	camera->m_ExposureStartY = chipUsedReadout->getYInt ();
	camera->m_ExposureNumX = getUsedWidthBinned ();
	camera->m_ExposureNumY = getUsedHeightBinned ();
	status = camera->GetImage ((short unsigned int*) getDataBuffer (0), width, height);
	logStream (MESSAGE_DEBUG) << "apogee doReadout status " << status << sendLog;
	if (!status)
		return -1;
	ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
	if (ret < 0)
		return -1;
	return -2;
}

int Apogee::endReadout ()
{
	camera->Reset ();
	camera->Flush ();
	return Camera::endReadout ();
}

//-------------------------------------------------------------
// CfgGet
//
// Retrieve a parameter from an INI file. Returns a status code
// and the paramter string in retbuff.
//-------------------------------------------------------------
bool Apogee::CfgGet (FILE * inifp, const char *inisect, const char *iniparm, char *retbuff, short bufflen, short *parmlen)
{
	short gotsect;
	char tbuf[256];
	char *ss, *eq, *ps, *vs, *ptr;

	rewind (inifp);

	// find the target section

	gotsect = 0;
	while (fgets (tbuf, 256, inifp) != NULL)
	{
		if ((ss = strchr (tbuf, '[')) != NULL)
		{
			if (strncasecmp (ss + 1, inisect, strlen (inisect)) == 0)
			{
				gotsect = 1;
				break;
			}
		}
	}

	if (!gotsect)
	{							 // section not found
		return false;
	}

	while (fgets (tbuf, 256, inifp) != NULL)
	{							 // find parameter in sect

								 // remove newline if there
		if ((ptr = strrchr (tbuf, '\n')) != NULL)
			*ptr = '\0';

								 // find the first non-blank
		ps = tbuf + strspn (tbuf, " \t");

		if (*ps == ';')			 // Skip line if comment
			continue;

		if (*ps == '[')
		{						 // Start of next section
			return false;
		}

		eq = strchr (ps, '=');	 // Find '=' sign in string

		if (eq)
								 // Find start of value str
			vs = eq + 1 + strspn (eq + 1, " \t");
		else
			continue;

		// found the target parameter

		if (strncasecmp (ps, iniparm, strlen (iniparm)) == 0)
		{

								 // cut off an EOL comment
			if ((ptr = strchr (vs, ';')) != NULL)
				*ptr = '\0';

			if (short (strlen (vs)) > bufflen - 1)
			{					 // not enough buffer space
				strncpy (retbuff, vs, bufflen - 1);
								 // put EOL in string
				retbuff[bufflen - 1] = '\0';
				*parmlen = bufflen;
				return true;
			}
			else
			{
								 // got it
				strcpy (retbuff, vs);
				ptr = retbuff;
				while (*ptr && *ptr != '\n' && *ptr != '\r')
					ptr++;
				*ptr = '\0';
				*parmlen = strlen (retbuff);
				return true;
			}
		}
	}

	return false;				 // parameter not found
}


// Initializes internal variables to their default value and reads the parameters in the
// specified INI file. Then initializes the camera using current settings. If BaseAddress
// or RegOffset parameters are specified (not equal to -1) then one or both of these
// values are used for the m_BaseAddress and m_RegisterOffset properties overriding those
// settings in the INI file.
int Apogee::config_load (short BaseAddress, short RegOffset)
{
	short plen;
	char retbuf[256];

	if ((strlen (cfgname) == 0) || (cfgname[0] == '\0'))
		return CCD_OPEN_CFGNAME;

	// attempt to open INI file
	FILE *inifp = NULL;

	if ((inifp = fopen (cfgname, "r")) == NULL)
		return CCD_OPEN_CFGDATA;

	// System
	if (CfgGet (inifp, "system", "interface", retbuf, sizeof (retbuf), &plen))
	{
		// Assume cam is currently null
		if (strcasecmp ("isa", retbuf) == 0)
		{
			return CCD_OPEN_NTIO;
		}
		else if (strcasecmp ("ppi", retbuf) == 0)
		{
			return CCD_OPEN_NTIO;
		}
		else if (strcasecmp ("pci", retbuf) == 0)
		{
			camera = new CCameraIO;
		}

		if (camera == NULL)
		{
			fclose (inifp);
			return CCD_OPEN_ALLOC;
		}
	}
	else
	{
		fclose (inifp);
		return CCD_OPEN_CFGDATA;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Settings which are stored in a class member (not in firmware) are already set
	// to a default value in the constructor. Settings accessed by get/put functions
	// must be set to a default value in this routine, after the base address and
	// communication protocal is setup.

	/////////////////////////////////////////////////////////////////////////////////
	// These settings must done first since they affect communication with the camera

	/*  if (BaseAddress == -1)
		{
		  if (CfgGet (inifp, "system", "base", retbuf, sizeof (retbuf), &plen))
	  camera->m_BaseAddress = strtol (retbuf, NULL, 0) & 0xFFF;
		  else
	  {
		fclose (inifp);
		delete camera;
		camera = NULL;
		return CCD_OPEN_CFGDATA;	// base address MUST be defined
	  }
		}
	  else
		camera->m_BaseAddress = BaseAddress & 0xFFF; */

	if (RegOffset == -1)
	{
		if (CfgGet
			(inifp, "system", "reg_offset", retbuf, sizeof (retbuf), &plen))
		{
			//        unsigned short val = strtol (retbuf, NULL, 0);
			//        if (val >= 0x0 && val <= 0xF0)
			//          camera->m_RegisterOffset = val & 0xF0;
		}
	}
	else
	{
		if (RegOffset >= 0x0 && RegOffset <= 0xF0)
			camera->m_RegisterOffset = RegOffset & 0xF0;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Necessary geometry settings

	if (CfgGet (inifp, "geometry", "rows", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXTOTALROWS)
			camera->m_Rows = val;
	}
	else
	{
		fclose (inifp);
		delete camera;
		camera = NULL;
		return CCD_OPEN_CFGDATA; // rows MUST be defined
	}

	if (CfgGet (inifp, "geometry", "columns", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, 0, 0);
		if (val >= 1 && val <= MAXTOTALCOLUMNS)
			camera->m_Columns = val;
	}
	else
	{
		fclose (inifp);
		delete camera;
		camera = NULL;
		return CCD_OPEN_CFGDATA; // columns MUST be defined
	}

	/////////////////////////////////////////////////////////////////////////////////

	if (CfgGet (inifp, "system", "pp_repeat", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val > 0 && val <= 1000)
			camera->m_PPRepeat = val;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// First actual communication with camera if in PPI mode
	if (!camera->InitDriver (device_id))
	{
		delete camera;
		camera = NULL;
		fclose (inifp);
		return CCD_OPEN_LOOPTST;
	}
	/////////////////////////////////////////////////////////////////////////////////
	// First actual communication with camera if in ISA mode
	camera->Reset ();			 // Read in command register to set shadow register known state
	/////////////////////////////////////////////////////////////////////////////////

	if (CfgGet (inifp, "system", "cable", retbuf, sizeof (retbuf), &plen))
	{
		if (!strcasecmp ("LONG", retbuf))
			camera->write_LongCable (true);
		else if (!strcasecmp ("SHORT", retbuf))
			camera->write_LongCable (false);
	}
	else
								 // default
		camera->write_LongCable (false);

	if (!camera->read_Present ())
	{
		delete camera;
		camera = NULL;
		fclose (inifp);

		return CCD_OPEN_LOOPTST;
	}
	/////////////////////////////////////////////////////////////////////////////////
	// Set default setting and read other settings from ini file

	camera->write_UseTrigger (false);
	camera->write_ForceShutterOpen (false);

	if (CfgGet
		(inifp, "system", "high_priority", retbuf, sizeof (retbuf), &plen))
	{
		if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
			|| !strcasecmp ("1", retbuf))
		{
			camera->m_HighPriority = true;
		}
		else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
			|| !strcasecmp ("0", retbuf))
		{
			camera->m_HighPriority = false;
		}
	}

	if (CfgGet (inifp, "system", "data_bits", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 8 && val <= 18)
			camera->m_DataBits = val;
	}

	if (CfgGet (inifp, "system", "sensor", retbuf, sizeof (retbuf), &plen))
	{
		if (strcasecmp ("ccd", retbuf) == 0)
		{
			camera->m_SensorType = Camera_SensorType_CCD;
		}
		else if (strcasecmp ("cmos", retbuf) == 0)
		{
			camera->m_SensorType = Camera_SensorType_CMOS;
		}
	}

	if (CfgGet (inifp, "system", "mode", retbuf, sizeof (retbuf), &plen))
	{
		unsigned short val = strtol (retbuf, NULL, 0) & 0xF;
		camera->write_Mode (val);
	}
	else
		camera->write_Mode (0);	 // default

	if (CfgGet (inifp, "system", "test", retbuf, sizeof (retbuf), &plen))
	{
		unsigned short val = strtol (retbuf, NULL, 0) & 0xF;
		camera->write_TestBits (val);
	}
	else
								 //default
		camera->write_TestBits (0);

	if (CfgGet (inifp, "system", "test2", retbuf, sizeof (retbuf), &plen))
	{
		unsigned short val = strtol (retbuf, NULL, 0) & 0xF;
		camera->write_Test2Bits (val);
	}
	else
								 // default
		camera->write_Test2Bits (0);

								 //default
	camera->write_FastReadout (false);

	if (CfgGet
		(inifp, "system", "shutter_speed", retbuf, sizeof (retbuf), &plen))
	{
		if (!strcasecmp ("normal", retbuf))
		{
			camera->m_FastShutter = false;
			camera->m_MaxExposure = 10485.75;
			camera->m_MinExposure = 0.01;
		}
		else if (!strcasecmp ("fast", retbuf))
		{
			camera->m_FastShutter = true;
			camera->m_MaxExposure = 1048.575;
			camera->m_MinExposure = 0.001;
		}
		else if (!strcasecmp ("dual", retbuf))
		{
			camera->m_FastShutter = true;
			camera->m_MaxExposure = 10485.75;
			camera->m_MinExposure = 0.001;
		}
	}

	if (CfgGet
		(inifp, "system", "shutter_bits", retbuf, sizeof (retbuf), &plen))
	{
		unsigned short val = strtol (retbuf, NULL, 0);
		camera->m_FastShutterBits_Mode = val & 0x0F;
		camera->m_FastShutterBits_Test = (val & 0xF0) >> 4;
	}

	if (CfgGet (inifp, "system", "maxbinx", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXHBIN)
			camera->m_MaxBinX = val;
	}

	if (CfgGet (inifp, "system", "maxbiny", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXVBIN)
			camera->m_MaxBinY = val;
	}

	if (CfgGet
		(inifp, "system", "guider_relays", retbuf, sizeof (retbuf), &plen))
	{
		if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
			|| !strcasecmp ("1", retbuf))
		{
			camera->m_GuiderRelays = true;
		}
		else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
			|| !strcasecmp ("0", retbuf))
		{
			camera->m_GuiderRelays = false;
		}
	}

	if (CfgGet (inifp, "system", "timeout", retbuf, sizeof (retbuf), &plen))
	{
		double val = atof (retbuf);
		if (val >= 0.0 && val <= 10000.0)
			camera->m_Timeout = val;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Geometry

	if (CfgGet (inifp, "geometry", "bic", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXCOLUMNS)
			camera->m_BIC = val;
	}

	if (CfgGet (inifp, "geometry", "bir", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXROWS)
			camera->m_BIR = val;
	}

	if (CfgGet (inifp, "geometry", "skipc", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 0 && val <= MAXCOLUMNS)
			camera->m_SkipC = val;
	}

	if (CfgGet (inifp, "geometry", "skipr", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 0 && val <= MAXROWS)
			camera->m_SkipR = val;
	}

	if (CfgGet (inifp, "geometry", "imgcols", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXTOTALCOLUMNS)
			camera->m_ImgColumns = val;
	}
	else
		camera->m_ImgColumns =
			camera->m_Columns - camera->m_BIC - camera->m_SkipC;

	if (CfgGet (inifp, "geometry", "imgrows", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXTOTALROWS)
			camera->m_ImgRows = val;
	}
	else
		camera->m_ImgRows = camera->m_Rows - camera->m_BIR - camera->m_SkipR;

	if (CfgGet (inifp, "geometry", "hflush", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXHBIN)
			camera->m_HFlush = val;
	}

	if (CfgGet (inifp, "geometry", "vflush", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= MAXVBIN)
			camera->m_VFlush = val;
	}

	// Default to full frame
	camera->m_NumX = camera->m_ImgColumns;
	camera->m_NumY = camera->m_ImgRows;

	/////////////////////////////////////////////////////////////////////////////////
	// Temperature

	if (CfgGet (inifp, "temp", "control", retbuf, sizeof (retbuf), &plen))
	{
		if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
			|| !strcasecmp ("1", retbuf))
		{
			camera->m_TempControl = true;
		}
		else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
			|| !strcasecmp ("0", retbuf))
		{
			camera->m_TempControl = false;
		}
	}

	if (CfgGet (inifp, "temp", "cal", retbuf, sizeof (retbuf), &plen))
	{
		short val = strtol (retbuf, NULL, 0);
		if (val >= 1 && val <= 255)
			camera->m_TempCalibration = val;
	}

	if (CfgGet (inifp, "temp", "scale", retbuf, sizeof (retbuf), &plen))
	{
		double val = atof (retbuf);
		if (val >= 1.0 && val <= 10.0)
			camera->m_TempScale = val;
	}

	if (CfgGet (inifp, "temp", "target", retbuf, sizeof (retbuf), &plen))
	{
		double val = atof (retbuf);
		if (val >= -60.0 && val <= 40.0)
			camera->write_CoolerSetPoint (val);
		else
			camera->write_CoolerSetPoint (-10.0);
	}
	else
								 //default
		camera->write_CoolerSetPoint (-10.0);

	/////////////////////////////////////////////////////////////////////////////////
	// CCD

	if (CfgGet (inifp, "ccd", "sensor", retbuf, sizeof (retbuf), &plen))
	{
		if (plen > 256)
			plen = 256;
		memcpy (camera->m_Sensor, retbuf, plen);
	}

	if (CfgGet (inifp, "ccd", "color", retbuf, sizeof (retbuf), &plen))
	{
		if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
			|| !strcasecmp ("1", retbuf))
		{
			camera->m_Color = true;
		}
		else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
			|| !strcasecmp ("0", retbuf))
		{
			camera->m_Color = false;
		}
	}

	if (CfgGet (inifp, "ccd", "noise", retbuf, sizeof (retbuf), &plen))
	{
		camera->m_Noise = atof (retbuf);
	}

	if (CfgGet (inifp, "ccd", "gain", retbuf, sizeof (retbuf), &plen))
	{
		camera->m_Gain = atof (retbuf);
	}

	if (CfgGet (inifp, "ccd", "pixelxsize", retbuf, sizeof (retbuf), &plen))
	{
		camera->m_PixelXSize = atof (retbuf);
	}

	if (CfgGet (inifp, "ccd", "pixelysize", retbuf, sizeof (retbuf), &plen))
	{
		camera->m_PixelYSize = atof (retbuf);
	}

	fclose (inifp);
	return CCD_OPEN_NOERR;
}

Apogee::Apogee (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	createTempSet ();
	createTempCCD ();

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

	createValue (coolerMode, "COOL_MOD", "cooler mode", true);
	coolerMode->addSelVal ("OFF");
	coolerMode->addSelVal ("ON");
	coolerMode->addSelVal ("SHUTDOWN");

	addOption ('n', "device_id", 1,	"device ID (ussualy 0, which is also default)");
	device_id = 0;
	addOption ('a', "config_name", 1, "device ini config file (default to /etc/rts2/apogee.ini");
	cfgname = "/etc/rts2/apogee.ini";

	camera = NULL;
}

Apogee::~Apogee (void)
{
	delete camera;
}

int Apogee::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'n':
			device_id = atoi (optarg);
			break;
		case 'a':
			cfgname = optarg;
			break;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}

int Apogee::init ()
{
	int ret;
	ret = Camera::init ();
	if (ret)
		return ret;

	Camera_Status status;

	ret = config_load (-1, -1);
	if (ret != CCD_OPEN_NOERR)
		return -1;

	camera->Flush ();

	status = camera->read_Status ();

	if (status < 0)
		return -1;

	ccdRealType->setValueCharArr (camera->m_Sensor);
	serialNumber->setValueCharArr ("007");

	return initChips ();
}

int Apogee::info ()
{
	coolerStatus->setValueInteger (camera->read_CoolerStatus ());
	coolerMode->setValueInteger (camera->read_CoolerMode ());
	tempSet->setValueDouble (camera->read_CoolerSetPoint ());
	tempCCD->setValueDouble (camera->read_Temperature ());
	return Camera::info ();
}

void Apogee::afterNight ()
{
	Camera_CoolerMode cMode;
	cMode = camera->read_CoolerMode ();
	camera->write_CoolerSetPoint (+20);
	// first shutdown, if we are in shutdown, then off
	if (cMode == Camera_CoolerMode_Shutdown)
		camera->write_CoolerMode (Camera_CoolerMode_Off);
	else
		camera->write_CoolerMode (Camera_CoolerMode_Shutdown);
}

int Apogee::setCoolTemp (float new_temp)
{
	Camera_CoolerMode cMode;
	cMode = camera->read_CoolerMode ();
	// we must traverse from shutdown to off mode before we can go to on
	if (cMode == Camera_CoolerMode_Shutdown)
		camera->write_CoolerMode (Camera_CoolerMode_Off);
	camera->write_CoolerSetPoint (new_temp);
	camera->write_CoolerMode (Camera_CoolerMode_On);
	return Camera::setCoolTemp (new_temp);
}

int main (int argc, char **argv)
{
	Apogee device (argc, argv);
	return device.run ();
}
