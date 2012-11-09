/*
 * Andor driver (optimised/specialised for iXon model
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iomanip>

#include "camd.h"

#ifdef __GNUC__
#  if(__GNUC__ > 3 || __GNUC__ ==3)
# define _GNUC3_
#  endif
#endif

#ifdef _GNUC3_
#  include <iostream>
#  include <fstream>
using namespace std;
#else
#  include <iostream.h>
#  include <fstream.h>
#endif

#include "atmcdLXd.h"
//That's root for andor2.77
#define ANDOR_ROOT          (char *) "/usr/local/etc/andor"

#define IXON_DEFAULT_GAIN   255
#define IXON_MAX_GAIN       255

#define ANDOR_SHUTTER_AUTO    0
#define ANDOR_SHUTTER_OPEN    1
#define ANDOR_SHUTTER_CLOSED  2

#define OPT_ANDOR_ROOT        OPT_LOCAL + 1

namespace rts2camd
{

/**
 * Andor camera, as seen by the outside world.
 *
 * This was originally written for an iXon, and doesn't handle anything
 * an iXon can't do.  If used with a different Andor camera, it should
 * respond reasonably well to the absence of iXon features
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Andor:public Camera
{
	public:
		Andor (int argc, char **argv);
		virtual ~Andor (void);

		virtual int initChips ();
		virtual int initHardware ();

		virtual bool supportFrameTransfer ();

		// callback functions for Camera alone
		virtual int info ();
		virtual int scriptEnds ();
		virtual int setCoolTemp (float new_temp);
		virtual void afterNight ();

	protected:
		virtual int processOption (int in_opt);
		virtual void help ();
		virtual void usage ();

		virtual void initDataTypes ();

		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual int doReadout ();

	private:
		char *andorRoot;
		bool printSpeedInfo;
		// number of AD channels
		int chanNum;

		AndorCapabilities cap;
		int andor_shutter_state;

		int disable_ft;
		int shutter_with_ft;

		rts2core::ValueSelection *tempStatus;

		rts2core::ValueInteger *gain;
		rts2core::ValueInteger *emccdgain;

		rts2core::ValueBool *useFT;
		rts2core::ValueBool *useRunTillAbort;

		rts2core::ValueSelection *VSAmp;
		rts2core::ValueBool *FTShutter;

		rts2core::ValueDouble *subExposure;

		// informational values
		rts2core::ValueSelection *ADChannel;
		rts2core::ValueBool *EMOn;
		rts2core::ValueInteger *HSpeed;
		rts2core::ValueSelection *VSpeed;
		rts2core::ValueFloat *HSpeedHZ;
		rts2core::ValueFloat *VSpeedHZ;

		rts2core::ValueInteger *bitDepth;
		rts2core::ValueInteger *acqusitionMode;

//		rts2core::ValueInteger *outputAmp;
		rts2core::ValueInteger *outPreAmpGain;

		rts2core::ValueBool *emAdvanced;
		rts2core::ValueSelection *emGainMode;
		rts2core::ValueSelection *fanMode;

		rts2core::ValueBool *filterCr;

		rts2core::ValueBool *baselineClamp;
		rts2core::ValueInteger *baselineOff;

		int defaultGain;

		void updateFlip ();

		void getTemp ();
		int setGain (int in_gain);
		int setEMCCDGain (int in_gain);
		int setADChannel (int in_adchan);
		int setVSAmplitude (int in_vsamp);
		int setHSSpeed (int in_amp, int in_hsspeed);
		int setVSSpeed (int in_vsspeed);
		int setFTShutter (bool force);
		int setUseFT (bool force);
		int setAcquisitionMode (int mode);

		int printInfo ();
		void printCapabilities ();
		int printNumberADCs ();
		int printHSSpeeds (int ad_channel, int amplifier);
		int updateHSSpeeds ();
		int printVSSpeeds ();

		void initAndorValues ();
		void closeShutter ();
};

}

using namespace rts2camd;

int Andor::stopExposure ()
{
	nullExposureConn ();
	AbortAcquisition ();
	FreeInternalMemory ();
	return Camera::stopExposure ();
}

long Andor::isExposing ()
{
	int status;
	int ret;
	// if we are in acqusition mode 5
	if (acqusitionMode->getValueInteger () == 5)
	{
		status = WaitForAcquisitionTimeOut (50);
		if (status == DRV_NO_NEW_DATA)
		{
			return -2;
		}
		logStream (MESSAGE_DEBUG) << "new image " << status << sendLog;

		// signal that we have data
		maskState (CAM_MASK_EXPOSE | CAM_MASK_READING | BOP_TEL_MOVE, CAM_NOEXPOSURE | CAM_READING, "chip extended readout started");
		// get the data
		at_32 lAcquired = 0;
		status = GetTotalNumberImagesAcquired (&lAcquired);
		logStream (MESSAGE_DEBUG) << "Circular buffer size " << lAcquired << " status " << status << sendLog;
		switch (getDataType ())
		{
			case RTS2_DATA_LONG:
				if (GetMostRecentImage ((at_32 *) getDataBuffer (0), chipUsedSize ()) != DRV_SUCCESS)
				{
					logStream (MESSAGE_ERROR) << "Cannot get long data" << sendLog;
					return -1;
				}
				break;
			default:
				if (GetMostRecentImage16 ((uint16_t *) getDataBuffer (0), chipUsedSize ()) != DRV_SUCCESS)
				{
					logStream (MESSAGE_ERROR) << "Cannot get int data" << sendLog;
					return -1;
				}
		}
		logStream (MESSAGE_DEBUG) << "Image "
			<< ((unsigned short *) getDataBuffer (0))[0] << " " 
			<< ((unsigned short *) getDataBuffer (0))[1] << " "
			<< ((unsigned short *) getDataBuffer (0))[2] << " "
			<< ((unsigned short *) getDataBuffer (0))[3] << " "
			<< ((unsigned short *) getDataBuffer (0))[4] << sendLog;
		// now send the data
		ret = sendImage (getDataBuffer (0), chipUsedSize ());
		if (ret)
			return ret;
		if (quedExpNumber->getValueInteger () == 0)
		{
			// stop exposure if we do not have any qued values
			AbortAcquisition ();
			FreeInternalMemory ();
			logStream (MESSAGE_INFO) << "Aborting acqusition" << sendLog;
			maskState (CAM_MASK_READING | BOP_TEL_MOVE, CAM_NOTREADING, "extended readout finished");
			return -3;
		}
		maskState (CAM_MASK_EXPOSE | CAM_MASK_READING | BOP_TEL_MOVE, CAM_EXPOSING | CAM_NOTREADING, "extended readout finished");
		quedExpNumber->dec ();
		sendValueAll (quedExpNumber);
		incExposureNumber ();
		return 100;
	}
	if ((ret = Camera::isExposing ()) != 0)
	{
		markReadoutStart ();
		return ret;
	}
	if (GetStatus (&status) != DRV_SUCCESS)
		return -1;
	if (status == DRV_ACQUIRING)
	{
		markReadoutStart ();
		return 100;
	}
	return -2;
}

// For each exposure, the first time this function is called, it reads out
// the entire image from the camera (into dest).  Subsequent calls return
// lines from dest.
int Andor::doReadout ()
{
	int status;
	if (GetStatus (&status) != DRV_SUCCESS)
		return -1;
	// if camera is still acquiring
	if (status == DRV_ACQUIRING)
	{
		return 0;
	}
	// 10 seconds timeout for acqusition
	if (getNow () > getExposureEnd () + 10)
	{
		logStream (MESSAGE_ERROR) << "exposure running for too long, aborting" << sendLog;
		AbortAcquisition ();
		return -1;
	}
	int ret;

	switch (getDataType ())
	{
		case RTS2_DATA_FLOAT:
			ret = GetAcquiredFloatData ((float *) getDataBuffer (0), chipUsedSize ());
			break;
		case RTS2_DATA_LONG:
			ret = GetAcquiredData ((at_32 *) getDataBuffer (0), chipUsedSize ());
			break;
			// case RTS2_DATA_SHORT:
		default:
			ret = GetAcquiredData16 ((short unsigned *) getDataBuffer (0), chipUsedSize ());
			break;
	}

	updateReadoutSpeed (getReadoutPixels ());

	if (ret != DRV_SUCCESS)
	{
		// acquisition in progress is NOT and error if it occurs early
		if ((ret == DRV_ACQUIRING) && (getNow () < getExposureEnd () + 100))
		{
			return 100;
		}
		logStream (MESSAGE_ERROR) << "andor GetAcquiredXXXX " << getDataType () << " return " << ret << sendLog;
		return -1;
	}
	ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
	if (ret < 0)
		return -1;
	if (getWriteBinaryDataSize () == 0)
		return -2;
	return 0;
}

bool Andor::supportFrameTransfer ()
{
	return useFT->getValueBool () && (cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER) && useRunTillAbort->getValueBool ();
}

void
Andor::closeShutter ()
{
	SetShutter (1, ANDOR_SHUTTER_CLOSED, 50, 50);
	andor_shutter_state = ANDOR_SHUTTER_CLOSED;
}

Andor::Andor (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	createTempCCD ();
	createTempSet ();

	createExpType ();

	andorRoot = ANDOR_ROOT;

	gain = NULL;
	VSAmp = NULL;
	baselineClamp = NULL;
	baselineOff = NULL;

	createValue (tempStatus, "temp_status", "Andor temperature status", false);
	tempStatus->addSelVal ("OFF");
	tempStatus->addSelVal ("NOT_STABILIZED");
	tempStatus->addSelVal ("STABILIZED");
	tempStatus->addSelVal ("NOT_REACHED");
	tempStatus->addSelVal ("OUT_RANGE");
	tempStatus->addSelVal ("NOT_SUPPORTED");
	tempStatus->addSelVal ("DRIFT");

	createValue (ADChannel, "ADCHANEL", "Used andor AD Channel", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	ADChannel->addSelVal ("14bit");
	ADChannel->addSelVal ("16bit");
	ADChannel->setValueInteger (0);

	createValue (VSpeed, "VSPEED", "Vertical shift speed", true, RTS2_VALUE_WRITABLE, CAM_WORKING);

	createValue (EMOn, "EMON", "Electron Multipliing status", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF, CAM_WORKING);
	EMOn->setValueBool (false);

	createValue (HSpeed, "HSPEED", "Horizontal shift speed", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	HSpeed->setValueInteger (0);

	createValue (FTShutter, "FTSHUT", "Use shutter, even with FT", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	FTShutter->setValueBool (false);

	createValue (subExposure, "subexposure", "subexposure length", true, RTS2_VALUE_WRITABLE, CAM_WORKING);

	createValue (useFT, "USEFT", "Use FT", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	useFT->setValueBool (true);

	createValue (useRunTillAbort, "USERTA", "Use run till abort mode of the CCD", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	// it is disabled, as it produces wrong data
	useRunTillAbort->setValueBool (false);

	createValue (acqusitionMode, "ACQMODE", "acqusition mode", true, 0, CAM_WORKING);

//	createValue (outputAmp, "OUTAMP", "output amplifier", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
//	outputAmp->setValueInteger (0);

	outPreAmpGain = NULL;

	emAdvanced = NULL;
	emGainMode = NULL;

	fanMode = NULL;

	createValue (filterCr, "FILTCR", "filter cosmic ray events", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	filterCr->setValueBool (false);

	defaultGain = IXON_DEFAULT_GAIN;

	printSpeedInfo = false;

	addOption (OPT_ANDOR_ROOT, "root", 1, "directory with Andor detector.ini file");
	addOption ('g', "gain", 1, "set camera gain level (0-255)");
	addOption ('N', "noft", 0, "do not use frame transfer mode");
	addOption ('I', "speed_info", 0,
		"print speed info - information about speed available");
	addOption ('S', "ft-uses-shutter", 0, "force use of shutter with FT");
}


Andor::~Andor (void)
{
	ShutDown ();
}

void Andor::help ()
{
	std::cout << "Driver for Andor CCDs (iXon & others)" << std::endl
		<< std::endl <<
		"\tOptimal values for speeds on iXon are: HSPEED=0 VSPEED=0. Those can be changed by modefile, specified by --modefile argument. There is an example modefile, which specify two modes:\n"
		<< std::endl << 
"[default]\n"
"EMON=on\n"
"HSPEED=1\n"
"VSPEED=1\n"
"\n"
"[classic]\n"
"EMON=off\n"
"HSPEED=3\n"
"VSPEED=1\n"

		<< std::endl;
	Camera::help ();
}

void Andor::usage ()
{
	std::cout << "\t" << getAppName () << " -c -70 -d C1" << std::endl;
}

int Andor::setGain (int in_gain)
{
	int ret;
	if ((ret = SetGain (in_gain)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setGain error " << ret << sendLog;
		return -1;
	}
	gain->setValueInteger (in_gain);
	return 0;
}

int Andor::setEMCCDGain (int in_gain)
{
	int ret;
	if ((ret = SetEMCCDGain (in_gain)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setEMCCDGain error " << ret << sendLog;
		return -1;
	}
	emccdgain->setValueInteger (in_gain);
	return 0;
}

int Andor::setADChannel (int in_adchan)
{
	int ret;
	if ((ret = SetADChannel (in_adchan)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setADChannel error " << ret <<
			sendLog;
		return -1;
	}
	ADChannel->setValueInteger (in_adchan);
	updateFlip ();
	return 0;
}

int Andor::setVSAmplitude (int in_vsamp)
{
	int ret;
	if ((ret = SetVSAmplitude (in_vsamp)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setVSAmplitude error " << ret << sendLog;
		return -1;
	}
	return 0;
}

int Andor::setHSSpeed (int in_amp, int in_hsspeed)
{
	int ret;
	// check if channel is correct
	int num;
	ret = GetNumberHSSpeeds (ADChannel->getValueInteger (), in_amp, &num);
	if (ret != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "cannot get number of horizontal shiwft speeds, error " << ret << sendLog;
		return -1;
	}
	if (in_hsspeed >= num)
	{
		logStream (MESSAGE_WARNING) << "cannot set horizontal shift speed to " << in_hsspeed
			<< ", changing request to " << (num - 1) << sendLog;
		in_hsspeed = num - 1;
		num = -1;
	}
	if ((ret = SetHSSpeed (in_amp, in_hsspeed)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setHSSpeed amplifier " << in_amp << " speed " << in_hsspeed << " error " << ret << sendLog;
		return -1;
	}
	EMOn->setValueBool (in_amp == 0 ? true : false);
	updateFlip ();
	HSpeed->setValueInteger (in_hsspeed);
	sendValueAll (HSpeed);
	if (num == -1)
		return -2;
	return 0;

}

int Andor::setVSSpeed (int in_vsspeed)
{
	int ret;
	if ((ret = SetVSSpeed (in_vsspeed)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setVSSpeed error " << ret << sendLog;
		return -1;
	}
	return 0;
}

int Andor::setFTShutter (bool force)
{
	FTShutter->setValueBool (force);
	return 0;
}

int Andor::setUseFT (bool force)
{
	int status;
	if (!(cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER))
		return -2;
	status = SetFrameTransferMode (force ? 1 : 0);
	if (status != DRV_SUCCESS)
		return -1;
	return 0;
}

int Andor::setAcquisitionMode (int mode)
{
	if (SetAcquisitionMode (mode) != DRV_SUCCESS)
		return -1;
	acqusitionMode->setValueInteger (mode);
	return 0;
}

void Andor::initDataTypes ()
{
	Camera::initDataTypes ();
	addDataType (RTS2_DATA_LONG);
	addDataType (RTS2_DATA_FLOAT);
}

int Andor::startExposure ()
{
	int ret;

	ret = SetImage (binningHorizontal (), binningVertical (), chipTopX () + 1,
		chipTopX () + chipUsedReadout->getHeightInt (),
		chipTopY () + 1,
		chipTopY () + chipUsedReadout->getWidthInt ());

	if (ret != DRV_SUCCESS)
	{
		if (ret == DRV_ACQUIRING)
		{
			quedExpNumber->inc ();
			return 0;
		}
		logStream (MESSAGE_ERROR) << "andor SetImage return " << ret << sendLog;
		return -1;
	}

	float acq_exp, acq_acc, acq_kinetic;
	if (isnan (subExposure->getValueDouble ()))
	{
		if (supportFrameTransfer ())
		{
			if (setAcquisitionMode (5))
			{
				logStream (MESSAGE_ERROR) << "Cannot set AQ run-till-abort mode" << sendLog;
				return -1;
			}
			if (SetKineticCycleTime (0) != DRV_SUCCESS)
			{
				logStream (MESSAGE_ERROR) << "Cannot set kinetic timing to 0" << sendLog;
				return -1;
			}
			if (GetAcquisitionTimings (&acq_exp, &acq_acc, &acq_kinetic) != DRV_SUCCESS)
				return -1;
		}
		else
		{
			// single scan
			if (setAcquisitionMode (AC_ACQMODE_SINGLE))
			{
				logStream (MESSAGE_ERROR) << "Cannot set AQ mode" << sendLog;
				return -1;
			}
		}
		if (SetExposureTime (getExposure ()) != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) << "Cannot set exposure time" << sendLog;
			return -1;
		}
	}
	else
	{
		nAcc = (int) (round (getExposure () / subExposure->getValueDouble ()));
		if (nAcc == 0)
		{
			nAcc = 1;
		}

		// Acquisition mode 2 is "accumulate"
		if (setAcquisitionMode (2))
			return -1;
		if (SetExposureTime (subExposure->getValueDouble ()) != DRV_SUCCESS)
			return -1;
		if (SetNumberAccumulations (nAcc) != DRV_SUCCESS)
			return -1;
		if (GetAcquisitionTimings (&acq_exp, &acq_acc, &acq_kinetic) != DRV_SUCCESS)
			return -1;
		setExposure (nAcc * acq_exp);
		subExposure->setValueDouble (acq_exp);
	}

	int new_state = (getExpType () == 0) ? ANDOR_SHUTTER_AUTO : ANDOR_SHUTTER_CLOSED;

	if ((getExpType () == 0) && useFT->getValueBool () && (!FTShutter->getValueBool ()))
		new_state = ANDOR_SHUTTER_OPEN;

	if (new_state != andor_shutter_state)
	{
		logStream (MESSAGE_DEBUG) << "SetShutter " << new_state << sendLog;
		ret = SetShutter (1, new_state, 50, 50);
		if (ret != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) << "Cannot set shutter state to " <<
				new_state << " error " << ret << sendLog;
			return -1;
		}
	}
	andor_shutter_state = new_state;

	getTemp ();

	if ((ret = StartAcquisition ()) != DRV_SUCCESS)
		return -1;
	return 0;
}

// scriptEnds
// Ensure that we definitely leave the shutter closed.
int Andor::scriptEnds ()
{
	if (!isnan (defaultGain) && gain)
		changeValue (gain, defaultGain);

	// set default values..
	changeValue (VSpeed, 1);
	changeValue (EMOn, false);
	changeValue (HSpeed, 0);

	changeValue (FTShutter, false);
	changeValue (useFT, true);

	changeValue (useRunTillAbort, false);
	
	//	closeShutter ();
	return Camera::scriptEnds ();
}

int Andor::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == gain)
		return setGain (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == emccdgain)
		return setEMCCDGain (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == ADChannel)
		return setADChannel (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == VSAmp)
		return setVSAmplitude (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == EMOn)
		return setHSSpeed (((rts2core::ValueBool *) new_value)->getValueBool () ? 0 : 1, HSpeed->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == HSpeed)
		return setHSSpeed (EMOn->getValueBool () ? 0 : 1, new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == VSpeed)
		return setVSSpeed (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == FTShutter)
		return setFTShutter (((rts2core::ValueBool *) new_value)->getValueBool ()) ==	0 ? 0 : -2;
	if (old_value == useFT)
	  	return setUseFT (((rts2core::ValueBool *) new_value)->getValueBool ()) == 0 ? 0 : -2;
	if (old_value == useRunTillAbort)
		return 0;
//	if (old_value == outputAmp)
//	{
//		return SetOutputAmplifier (new_value->getValueInteger ()) == DRV_SUCCESS ? 0 : -2;
//	}
	if (old_value == outPreAmpGain)
	{
		return SetPreAmpGain (new_value->getValueInteger ()) == DRV_SUCCESS ? 0 : -2;
	}
	if (old_value == filterCr)
	{
		return SetFilterMode (((rts2core::ValueBool *)new_value)->getValueBool () ? 2 : 0) == DRV_SUCCESS ? 0 : -2;
	}
	if (old_value == baselineClamp)
	{
		return SetBaselineClamp (((rts2core::ValueBool *)new_value)->getValueBool () ? 1 : 0) == DRV_SUCCESS ? 0 : -2;
	}
	if (old_value == baselineOff)
	{
		return SetBaselineOffset (new_value->getValueInteger ()) == DRV_SUCCESS ? 0 : -2;
	}
	if (old_value == emAdvanced)
	{
		return SetEMAdvanced (((rts2core::ValueBool *)new_value)->getValueBool () ? 1 : 0) == DRV_SUCCESS ? 0 : -2;
	}
	if (old_value == emGainMode)
	{
		return SetEMGainMode (new_value->getValueInteger ()) == DRV_SUCCESS ? 0 : -2;
	}
	if (old_value == fanMode)
	{
		return SetFanMode (new_value->getValueInteger ()) == DRV_SUCCESS ? 0 : -2;
	}

	return Camera::setValue (old_value, new_value);
}

int Andor::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'g':
			defaultGain = atoi (optarg);
			if (defaultGain > IXON_MAX_GAIN || defaultGain < 0)
			{
				printf ("gain must be in 0-255 range\n");
				exit (EXIT_FAILURE);
			}
			break;
		case OPT_ANDOR_ROOT:
			andorRoot = optarg;
			break;
		case 'I':
			printSpeedInfo = true;
			break;
		case 'N':
			useFT->setValueBool (false);
			break;
		case 'S':
			shutter_with_ft = true;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}


/*******************************************************************
 * printInfo (multiple functions)
 *
 * Do a full probe of what the attached camera can do, and print it out.
 * Note that an amount of stuff is duplicated between here and CameraAndorChip
 * so if/when that gets merged, some info may aready be available.
 *
 */
void Andor::printCapabilities ()
{
	printf ("Acquisition modes: ");
	if (cap.ulAcqModes == 0)
		printf ("<none>");
	if (cap.ulAcqModes & AC_ACQMODE_SINGLE)
		printf (" SINGLE");
	if (cap.ulAcqModes & AC_ACQMODE_VIDEO)
		printf (" VIDEO");
	if (cap.ulAcqModes & AC_ACQMODE_ACCUMULATE)
		printf (" ACCUMULATE");
	if (cap.ulAcqModes & AC_ACQMODE_KINETIC)
		printf (" KINETIC");
	if (cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER)
		printf (" FRAMETRANSFER");
	if (cap.ulAcqModes & AC_ACQMODE_FASTKINETICS)
		printf (" FASTKINETICS");

	printf ("\nRead modes: ");
	if (cap.ulReadModes & AC_READMODE_FULLIMAGE)
		printf (" FULLIMAGE");
	if (cap.ulReadModes & AC_READMODE_SUBIMAGE)
		printf (" SUBIMAGE");
	if (cap.ulReadModes & AC_READMODE_SINGLETRACK)
		printf (" SINGLETRACK");
	if (cap.ulReadModes & AC_READMODE_FVB)
		printf (" FVB");
	if (cap.ulReadModes & AC_READMODE_MULTITRACK)
		printf (" MULTITRACK");
	if (cap.ulReadModes & AC_READMODE_RANDOMTRACK)
		printf (" RANDOMTRACK");

	printf ("\nTrigger modes: ");
	if (cap.ulTriggerModes & AC_TRIGGERMODE_INTERNAL)
		printf (" INTERNAL");
	if (cap.ulTriggerModes & AC_TRIGGERMODE_EXTERNAL)
		printf (" EXTERNAL");

	printf ("\nPixel modes: ");
	if (cap.ulPixelMode & AC_PIXELMODE_8BIT)
		printf (" 8BIT");
	if (cap.ulPixelMode & AC_PIXELMODE_14BIT)
		printf (" 14BIT");
	if (cap.ulPixelMode & AC_PIXELMODE_16BIT)
		printf (" 16BIT");
	if (cap.ulPixelMode & AC_PIXELMODE_32BIT)
		printf (" 32BIT");
	if (cap.ulPixelMode & AC_PIXELMODE_MONO)
		printf (" MONO");
	if (cap.ulPixelMode & AC_PIXELMODE_RGB)
		printf (" RGB");
	if (cap.ulPixelMode & AC_PIXELMODE_CMY)
		printf (" CMY");

	printf ("\nSettable variables: ");
	if (cap.ulSetFunctions & AC_SETFUNCTION_VREADOUT)
		printf (" VREADOUT");
	if (cap.ulSetFunctions & AC_SETFUNCTION_HREADOUT)
		printf (" HREADOUT");
	if (cap.ulSetFunctions & AC_SETFUNCTION_TEMPERATURE)
		printf (" TEMPERATURE");
	if (cap.ulSetFunctions & AC_SETFUNCTION_GAIN)
		printf (" GAIN");
	if (cap.ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN)
		printf (" EMCCDGAIN");

	printf ("\n");
}

/****************************************************************
 * printNumberADCs
 *
 * Prints out number and bit-depth of available AD channels, and returns
 * the number of AD channels found, 0 on error.
 */
int Andor::printNumberADCs ()
{
	int ret, n_ad;
	if ((ret = GetNumberADChannels (&n_ad)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "cannot get number of AD channels" << sendLog;
		return -1;
	}
	std::cout << "AD Channels: " << n_ad << " (";
	for (int ad = 0; ad < n_ad; ad++)
	{
		int depth;
		if ((ret = GetBitDepth (ad, &depth)) != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) << "cannot retrieve get depth for ad " << ad << sendLog;
			return -1;
		}
		if (n_ad > 1)
			std::cout << ad << "=" << depth << "-bit";
		else
			std::cout << depth << "-bit";
		if (ad == (n_ad - 1))
			std::cout << ")" << std::endl;
		else
			std::cout << ", ";
	}
	return n_ad;
}

int Andor::printHSSpeeds (int ad, int amp)
{
	int ret;
	int nhs, npreamps;
	if ((ret = GetNumberHSSpeeds (ad, amp, &nhs)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "cannot retrieve number of horizontal speeds" << sendLog;
		return -1;
	}

	if ((ret = GetNumberPreAmpGains (&npreamps)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "cannot retrieve number of preAmps gains" << sendLog;
		return -1;
	}

	std::cout << "Horizontal speeds for AD " << ad << " and amplifier " << amp << ": " << nhs << std::endl;

	for (int s = 0; s < nhs; s++)
	{
		float val;
		if ((ret = GetHSSpeed (ad, amp, s, &val)) != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) <<
				"andor cannot get horizontal speed " << s <<
				" ad " << ad << " amp " << amp << sendLog;
			return -1;
		}
		std::cout << val;
		switch (cap.ulCameraType)
		{
			case AC_CAMERATYPE_IXON:
				std::cout << " MHz)" << std::endl;
				break;
			default:
				std::cout << " usec/pix)" << std::endl;
				break;
		}
		// prints available preAmpGains
		for (int p = 0; p < npreamps; p++)
		{
			float preAmpGain;
			int isPreAmp;
			GetPreAmpGain (p, &preAmpGain);
			IsPreAmpGainAvailable (ad, amp, s, p, &isPreAmp);
			std::cout << "  pream " << p
				<< " preampVal " << preAmpGain
				<< " is " << isPreAmp
				<< " " << std::endl;
		}
	}

	return 0;
}

int Andor::printVSSpeeds ()
{
	int ret, vspeeds;
	if ((ret = GetNumberVSSpeeds (&vspeeds)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor init cannot get vertical speeds" << sendLog;
		return -1;
	}
	std::cout << "Vertical Speeds: " << vspeeds << " (";
	for (int s = 0; s < vspeeds; s++)
	{
		float val;
		GetVSSpeed (s, &val);
		std::cout << val;
		if (s == (vspeeds - 1))
			std::cout << " usec/pix)" << std::endl;
		else
			std::cout << ", ";
	}
	return 0;
}

int Andor::printInfo ()
{
	int ret;
	int n_ad, n_amp;
	char name[128];

	printf ("Camera type: ");
	switch (cap.ulCameraType)
	{
		case AC_CAMERATYPE_PDA:
			printf ("PDA");
			break;
		case AC_CAMERATYPE_IXON:
			printf ("IXON");
			break;
		case AC_CAMERATYPE_ICCD:
			printf ("ICCD");
			break;
		case AC_CAMERATYPE_EMCCD:
			printf ("EMCCD");
			break;
		case AC_CAMERATYPE_CCD:
			printf ("CCD");
			break;
		case AC_CAMERATYPE_ISTAR:
			printf ("ISTAR");
			break;
		case AC_CAMERATYPE_VIDEO:
			printf ("VIDEO");
			break;
		default:
			printf ("<unknown> (code is %li)", cap.ulCameraType);
			break;
	}

	GetHeadModel (name);
	printf (" Model: %s\n", name);

	printCapabilities ();

	if ((n_ad = printNumberADCs ()) < 1)
		return -1;

	GetNumberAmp (&n_amp);
	printf ("Output amplifiers: %d\n", n_amp);

	for (int ad = 0; ad < n_ad; ad++)
		for (int amp = 0; amp < n_amp; amp++)
			if ((ret = printHSSpeeds (ad, amp)) != 0)
				return ret;

	if ((ret = printVSSpeeds ()) != 0)
		return ret;

	return 0;
}

int Andor::initChips ()
{
	int ret;
	float x_um, y_um;
	int x_pix, y_pix;

	SetShutter (1, ANDOR_SHUTTER_CLOSED, 50, 50);
	andor_shutter_state = ANDOR_SHUTTER_CLOSED;

	if ((ret = GetPixelSize (&x_um, &y_um)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) <<
			"andor chip cannot get pixel size" << ret << sendLog;
		return -1;
	}

	//GetPixelSize returns floats, pixel[XY] are doubles
	pixelX = x_um;
	pixelY = y_um;

	if ((ret = GetDetector (&x_pix, &y_pix)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) <<
			"andor chip cannot get detector size" << ret << sendLog;
		return -1;
	}
	setSize (x_pix, y_pix, 0, 0);

	if ((ret = GetCapabilities (&cap)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) <<
			"andor chip failed to retrieve camera capabilities" << ret << sendLog;
		return -1;
	}

	// use frame transfer mode
	if (useFT->getValueBool ()
		&& (cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER)
		&& ((ret = SetFrameTransferMode (1)) != DRV_SUCCESS))
	{
		logStream (MESSAGE_ERROR) << "andor init attempt to set frame transfer failed " << ret << sendLog;
		return -1;
	}

	// init vspeed selection
	int vspeeds;

	if ((ret = GetNumberVSSpeeds (&vspeeds)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "cannot retrieve number of VSpeeds" << sendLog;
		return -1;
	}

	for (int i = 0; i < vspeeds; i++)
	{
		std::ostringstream os;
		float val;
		GetVSSpeed (i, &val);
		os << val << " usec/pix";
		VSpeed->addSelVal (os.str ());
	}

	return 0;
}

int Andor::initHardware ()
{
	unsigned long err;
	int ret;

	ret = rts2core::Device::doDaemonize ();
	if (ret)
		exit (ret);

	err = Initialize (andorRoot);
	if (err != DRV_SUCCESS)
	{
		cerr << "Andor library init failed (code " << err << "). exiting" <<
			endl;
		return -1;
	}

	sleep (2);					 //sleep to allow initialization to complete

	SetExposureTime (5.0);

	ret = GetCapabilities (&cap);
	if (ret != DRV_SUCCESS)
	{
		cerr << "Cannot call GetCapabilities " << ret << endl;
		return -1;
	}


	// iXon+ cameras can do "real gain", indicated by a flag.
	if (cap.ulEMGainCapability & 0x08)
	{
		ret = SetEMGainMode(3);
		if (ret != DRV_SUCCESS)
		{
			cerr << "Failed to set real gain mode, error " << ret << endl;
			return -1;
		}
	}
	initAndorValues ();

	if (baselineClamp != NULL)
	{
		// This clamps the baseline level of kinetic series frames.  Didn't exist
		// in older SDK versions, is not a problem for non-KS exposures.
		ret = SetBaselineClamp(0);
		if (ret != DRV_SUCCESS)
		{
			cerr << "Failed to set baseline clamp, error " << ret << endl;
			return -1;
		}
	}


	//Set Read Mode to --Image--
	ret = SetReadMode (4);
	if (ret != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "Cannot set read mode (" << ret << "), exiting" << sendLog;
		return -1;
	}

	if (printSpeedInfo)
		printInfo ();

	ccdRealType->setValueCharArr ("ANDOR");

	int serNum;
	ret = GetCameraSerialNumber (&serNum);
	if (ret != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "cannot get serial number" << sendLog;
		return -1;
	}
	serialNumber->setValueCharArr (serNum);

	// default to EMON off
	ret = setHSSpeed (1, HSpeed->getValueInteger ());
	if (ret != 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot set non-EM mode (the default)" << sendLog;
		return -1;
	}

	ret = setADChannel (1);
	if (ret != 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot set default AD Channel to 1" << sendLog;
		return -1;
	}

	return initChips ();
}

void Andor::initAndorValues ()
{
	if (cap.ulSetFunctions & AC_SETFUNCTION_VSAMPLITUDE)
	{
		createValue (VSAmp, "SAMPLI", "Used andor shift amplitude", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		VSAmp->addSelVal ("normal");
		VSAmp->addSelVal ("+1");
		VSAmp->addSelVal ("+2");
		VSAmp->addSelVal ("+3");
		VSAmp->addSelVal ("+4");
		VSAmp->setValueInteger (0);
	}
	if (cap.ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN)
	{
		createValue (outPreAmpGain, "PREAMP", "output preamp gain", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		outPreAmpGain->setValueInteger (0);
	}
	if (cap.ulSetFunctions & AC_SETFUNCTION_GAIN)
	{
		createValue (gain, "GAIN", "CCD gain", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		setGain (defaultGain);
	}
	if (cap.ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN)
	{
		createValue (emccdgain, "EMCCDGAIN", "EM CCD gain", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		setEMCCDGain (defaultGain);

		if (cap.ulEMGainCapability != 0)
		{
			createValue (emAdvanced, "EMADV", "advanced EM mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
			emAdvanced->setValueBool (false);

			createValue (emGainMode, "GAINMODE", "EM gain mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
			emGainMode->addSelVal ("DAC 8bit");
			emGainMode->addSelVal ("DAC 12bit");
			emGainMode->addSelVal ("LINEAR 12bit");
			emGainMode->addSelVal ("REAL 12bit");
		}
	}
	if (cap.ulSetFunctions & AC_SETFUNCTION_BASELINECLAMP)
	{
		createValue (baselineClamp, "BASECLAM", "if baseline clamp is activer", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		baselineClamp->setValueBool (false);
	}
	if (cap.ulSetFunctions & AC_SETFUNCTION_BASELINEOFFSET)
	{
		createValue (baselineOff, "BASEOFF", "baseline offset value", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		baselineOff->setValueInteger (0);
	}
	if (cap.ulFeatures & AC_FEATURES_FANCONTROL)
	{
		createValue (fanMode, "FANMODE", "FAN mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		fanMode->addSelVal ("FULL");
		fanMode->addSelVal ("LOW");
		fanMode->addSelVal ("OFF");
	}
}

void Andor::updateFlip ()
{
	if (EMOn->getValueBool () == false)
	{
		switch (ADChannel->getValueInteger ())
		{
			case 0:
				changeAxisDirections (true, true);
				break;
			case 1:
				changeAxisDirections (false, false);
				break;
		}
	}
	else
	{
		switch (ADChannel->getValueInteger ())
		{
			case 0:
				changeAxisDirections (true, true);
				break;
			case 1:
				changeAxisDirections (false, false);
				break;
		}
	}
}

void Andor::getTemp ()
{
	int c_status;
	float tmpTemp;
	c_status = GetTemperatureF (&tmpTemp);
	if (!(c_status == DRV_ACQUIRING || c_status == DRV_NOT_INITIALIZED || c_status == DRV_ERROR_ACK))
	{
		tempCCD->setValueDouble (tmpTemp);
		switch (c_status)
		{
			case DRV_TEMPERATURE_OFF:
				tempStatus->setValueInteger (0);
				break;
			case DRV_TEMPERATURE_NOT_STABILIZED:
				tempStatus->setValueInteger (1);
				break;
			case DRV_TEMPERATURE_STABILIZED:
				tempStatus->setValueInteger (2);
				break;
			case DRV_TEMPERATURE_NOT_REACHED:
				tempStatus->setValueInteger (3);
				break;
			case DRV_TEMPERATURE_OUT_RANGE:
				tempStatus->setValueInteger (4);
				break;
			case DRV_TEMPERATURE_NOT_SUPPORTED:
				tempStatus->setValueInteger (5);
				break;
			case DRV_TEMPERATURE_DRIFT:
				tempStatus->setValueInteger (6);
				break;
			default:
				logStream (MESSAGE_WARNING) << "unknow temperature status " << c_status << sendLog;
		}
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "andor info status " << c_status <<
			sendLog;
	}
}

int Andor::info ()
{
	if (isIdle ())
	{
		getTemp ();
	}
	return Camera::info ();
}

int Andor::setCoolTemp (float new_temp)
{
	int status;
	status = CoolerON ();
	if (status != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "Cannot switch cooler to on, status: " << status << sendLog;
		return -1;
	}
	status = SetTemperature ((int) new_temp);
	if (status != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "Cannot set cooling tempereture, status: " << status << sendLog;
		return -1;
	}
	return Camera::setCoolTemp (new_temp);
}

void Andor::afterNight ()
{
	CoolerOFF ();
	SetTemperature (20);
	tempSet->setValueDouble (+50);
	closeShutter ();
}

int main (int argc, char **argv)
{
	Andor device (argc, argv);
	return device.run ();
}
