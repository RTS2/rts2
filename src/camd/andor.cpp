/***************************************************************
 * Andor driver (optimised/specialised for iXon model
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
//We need at least version 2.7.  2.6 API doesn't have frame transfer
//#define ANDOR_ROOT "/root/andor2.7/examples/common"

//That's root for andor2.77
#define ANDOR_ROOT "/usr/local/etc/andor"

#define IXON_DEFAULT_GAIN 255
#define IXON_MAX_GAIN 255

#define ANDOR_SHUTTER_AUTO 0
#define ANDOR_SHUTTER_OPEN 1
#define ANDOR_SHUTTER_CLOSED 2

// Mode definitions for the BOOTES projects iXon camera.
// Arguably they shouldn't be here, but....
typedef struct ixon_mode_t
{
	int ad;						 ///0=14bit (3,5,10MHz), 1=16bit (1MHz)
	int hsspeed;				 ///speed setting (varies with ADC, see docs)
	int vsspeed;				 ///
	int vs_amp;					 ///driving voltage, 0-4, leave low if possible
	int disable_em;				 ///0 = use EMCCD
} ixon_mode;

#define IXON_MODES 7
const ixon_mode mode_def[] =	 //default with which we worked
{
	{
		1, 0, 5, 1, 0
	},
	{							 //16 bit @ 1MHz
		1, 0, 0, 1, 0
	},
	{							 //14 bit @ 3MHz
		0, 2, 0, 1, 0
	},
	{							 //14 bit @ 5MHz
		0, 1, 0, 1, 0
	},
	{							 //14 bit @ 10MHz
		0, 0, 0, 1, 0
	},
	{							 //16 bit @ 1MHz, no em
		1, 0, 0, 1, 1
	},
	{							 //14 bit @ 3MHz, no em
		0, 0, 0, 1, 1
	}
};

/***********************************************************************/
/**
 * Andor Camera Chip
 *
 * In this case, handles memory and some startup hardware poking
 */

class CameraAndorChip:public CameraChip
{
	private:
		AndorCapabilities cap;
		unsigned short *dest;	 /// Memory array for reading out
		unsigned short *dest_top;
		char *send_top;
		int gain;
		bool useFT;
		int andor_shutter_state;

	public:
		CameraAndorChip (Rts2DevCamera * in_cam, int in_chip_id,
			int in_gain, bool in_useFT);
		virtual ~ CameraAndorChip (void);
		virtual int init ();
		virtual int startExposure (int light, float exptime);
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
		virtual int readoutOneLine ();
		virtual bool supportFrameTransfer ();

		int setUseFT (bool in_useFT);

		// *FIXME* only CameraAndor needs to play with this, do we need functions?
		int use_shutter_anyway;	 // Use shutter even if we have FT
		void closeShutter ();
};

CameraAndorChip::CameraAndorChip (Rts2DevCamera * in_cam, int in_chip_id,
int in_gain, bool in_useFT):
CameraChip (in_cam, in_chip_id)
{
	gain = in_gain;
	useFT = in_useFT;
}


CameraAndorChip::~CameraAndorChip (void)
{
	delete dest;
};

/** init()
 * Should not be called until post andor Initialise() call
 * Finds out various pieces of information from the Andor library,
 * and switches on frame transfer if the camera has it.
 */
int
CameraAndorChip::init ()
{
	int ret;
	float x_um, y_um;
	int x_pix, y_pix;

	SetShutter (1, ANDOR_SHUTTER_CLOSED, 50, 50);
	andor_shutter_state = ANDOR_SHUTTER_CLOSED;
	use_shutter_anyway = 0;

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
	dest = new unsigned short[x_pix * y_pix];

	if ((ret = GetCapabilities (&cap)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) <<
			"andor chip failed to retrieve camera capabilities" << ret << sendLog;
		return -1;
	}

	// use frame transfer mode
	if ((cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER) && (useFT) &&
		((ret = SetFrameTransferMode (1)) != DRV_SUCCESS))
	{
		logStream (MESSAGE_ERROR) <<
			"andor init attempt to set frame transfer failed " << ret << sendLog;
		return -1;
	}

	return CameraChip::init ();
}


int
CameraAndorChip::startExposure (int light, float exptime)
{
	int ret;

	ret =
		SetImage (binningHorizontal, binningVertical, chipReadout->x + 1,
		chipReadout->x + chipReadout->height, chipReadout->y + 1,
		chipReadout->y + chipReadout->width);
	if (ret != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor SetImage return " << ret << sendLog;
		return -1;
	}

	subExposure = camera->getSubExposure ();
	if (isnan (subExposure))
	{
		// single scan
		if (SetAcquisitionMode (AC_ACQMODE_SINGLE) != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) << "Cannot set AQ mode" << sendLog;
			return -1;
		}
		if (SetExposureTime (exptime) != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) << "Cannot set exposure time" << sendLog;
			return -1;
		}
	}
	else
	{
		nAcc = (int) (exptime / subExposure);
		float acq_exp, acq_acc, acq_kinetic;
		if (nAcc == 0)
		{
			nAcc = 1;
			subExposure = exptime;
		}

		// Acquisition mode 2 is "accumulate"
		if (SetAcquisitionMode (2) != DRV_SUCCESS)
			return -1;
		if (SetExposureTime (subExposure) != DRV_SUCCESS)
			return -1;
		if (SetNumberAccumulations (nAcc) != DRV_SUCCESS)
			return -1;
		if (GetAcquisitionTimings (&acq_exp, &acq_acc, &acq_kinetic) !=
			DRV_SUCCESS)
			return -1;
		exptime = nAcc * acq_exp;
		subExposure = acq_exp;
		camera->setSubExposure (subExposure);
	}

	chipUsedReadout = new ChipSubset (chipReadout);
	usedBinningVertical = binningVertical;
	usedBinningHorizontal = binningHorizontal;

	int new_state = (light) ? ANDOR_SHUTTER_AUTO : ANDOR_SHUTTER_CLOSED;

	if ((light) && (useFT) && (!use_shutter_anyway))
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

	if ((ret = StartAcquisition ()) != DRV_SUCCESS)
		return -1;
	return 0;
}


int
CameraAndorChip::stopExposure ()
{
	AbortAcquisition ();
	FreeInternalMemory ();
	return CameraChip::stopExposure ();
}


long
CameraAndorChip::isExposing ()
{
	int status;
	int ret;
	if ((ret = CameraChip::isExposing ()) != 0)
		return ret;
	if (GetStatus (&status) != DRV_SUCCESS)
		return -1;
	if (status == DRV_ACQUIRING)
		return 100;
	return 0;
}


int
CameraAndorChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
	dest_top = dest;
	send_top = (char *) dest;
	return CameraChip::startReadout (dataConn, conn);
}


/***************************************************************
 * readoutOneLine
 *
 * For each exposure, the first time this function is called, it reads out
 * the entire image from the camera (into dest).  Subsequent calls return
 * lines from dest.
 */

int
CameraAndorChip::readoutOneLine ()
{
	int ret;
	if (readoutLine < chipUsedReadout->height)
	{
		int size =
			(chipUsedReadout->width / usedBinningHorizontal) *
			(chipUsedReadout->height / usedBinningVertical);
		ret = GetAcquiredData16 (dest, size);
		if (ret != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) << "andor GetAcquiredData16 return " <<
				ret << sendLog;
			return -1;
		}
		readoutLine = chipUsedReadout->height;
		dest_top += size;
		return 0;
	}
	if (sendLine == 0)
	{
		ret = CameraChip::sendFirstLine ();
		if (ret)
			return ret;
	}
	int send_data_size;
	sendLine++;
	send_data_size = sendReadoutData (send_top, (char *) dest_top - send_top);
	if (send_data_size < 0)
		return -1;
	send_top += send_data_size;
	if (send_top < (char *) dest_top)
		return 0;
	return -2;
}


bool
CameraAndorChip::supportFrameTransfer ()
{
	return (cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER);
}


int
CameraAndorChip::setUseFT (bool in_useFT)
{
	int status;
	if (!supportFrameTransfer ())
		return -1;
	status = SetFrameTransferMode (in_useFT ? 1 : 0);
	if (status != DRV_SUCCESS)
		return -1;
	useFT = in_useFT;
	return 0;
}


void
CameraAndorChip::closeShutter ()
{
	SetShutter (1, ANDOR_SHUTTER_CLOSED, 50, 50);
	andor_shutter_state = ANDOR_SHUTTER_CLOSED;
}


/***********************************************************************
 ***********************************************************************/

/**
 * Andor camera, as seen by the outside world.
 *
 * This was originally written for an iXon, and doesn't handle anything
 * an iXon can't do.  If used with a different Andor camera, it should
 * respond reasonably well to the absence of iXon features
 *
 ************************************************************************
 ***********************************************************************/

class Rts2DevCameraAndor:public Rts2DevCamera
{
	private:
		char *andorRoot;
		bool printSpeedInfo;
		// number of AD channels
		int chanNum;

		int disable_ft;
		int shutter_with_ft;
		int mode;

		AndorCapabilities cap;

		Rts2ValueInteger *gain;

		Rts2ValueInteger *Mode;
		Rts2ValueBool *useFT;
		Rts2ValueInteger *VSAmp;
		Rts2ValueBool *FTShutter;

		// informational values
		Rts2ValueInteger *ADChannel;
		Rts2ValueBool *EMOn;
		Rts2ValueInteger *HSpeed;
		Rts2ValueInteger *VSpeed;
		Rts2ValueFloat *HSpeedHZ;
		Rts2ValueFloat *VSpeedHZ;
		Rts2ValueInteger *bitDepth;

		int defaultGain;

		void getTemp ();
		int setGain (int in_gain);
		int setADChannel (int in_adchan);
		int setVSAmplitude (int in_vsamp);
		int setHSSpeed (int in_amp, int in_hsspeed);
		int setVSSpeed (int in_vsspeed);
		int setFTShutter (int force);
		int setMode (int mode);

		int printInfo ();
		void printCapabilities ();
		int printNumberADCs ();
		int printHSSpeeds (int camera_type, int ad_channel, int amplifier);
		int printVSSpeeds ();

		void initAndorValues ();

	protected:
		virtual int processOption (int in_opt);
		virtual void help ();
		virtual void cancelPriorityOperations ();
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		Rts2DevCameraAndor (int argc, char **argv);
		virtual ~ Rts2DevCameraAndor (void);

		virtual int init ();

		// callback functions for Camera alone
		virtual int ready ();
		virtual int info ();
		virtual int scriptEnds ();
		virtual int camChipInfo (int chip);
		virtual int camCoolMax ();
		virtual int camCoolHold ();
		virtual int camCoolTemp (float new_temp);
		virtual int camCoolShutdown ();

		virtual int camExpose (int chip, int light, float exptime);
};

Rts2DevCameraAndor::Rts2DevCameraAndor (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
	createTempCCD ();
	createTempRegulation ();
	createTempSet ();

	andorRoot = "/root/andor/examples/common";

	gain = NULL;

	createValue (Mode, "MODE", "Camera mode", true, 0,
		CAM_EXPOSING | CAM_READING | CAM_DATA, true);
	Mode->setValueInteger (0);
	createValue (ADChannel, "ADCHANEL",
		"Used andor AD Channel, on ixon 0 for 14 bit, 1 for 16 bit",
		true, 0, CAM_EXPOSING | CAM_READING | CAM_DATA, true);
	ADChannel->setValueInteger (0);
	// create VSAmp only if we have amplitude set capability
	VSAmp = NULL;
	createValue (VSpeed, "VSPEED", "Vertical shift speed", true, 0,
		CAM_EXPOSING | CAM_READING | CAM_DATA, true);
	VSpeed->setValueInteger (1);
	createValue (EMOn, "EMON", "If EM is enabled", true, 0,
		CAM_EXPOSING | CAM_READING | CAM_DATA, true);
	EMOn->setValueBool (true);
	createValue (HSpeed, "HSPEED", "Horizontal shift speed", true, 0,
		CAM_EXPOSING | CAM_READING | CAM_DATA, true);
	HSpeed->setValueInteger (1);
	createValue (FTShutter, "FTSHUT", "Use shutter, even with FT", false, 0,
		CAM_EXPOSING | CAM_READING | CAM_DATA, true);
	FTShutter->setValueBool (false);

	createValue (useFT, "USEFT", "Use FT", false, 0,
		CAM_EXPOSING | CAM_READING | CAM_DATA, true);
	useFT->setValueBool (true);

	defaultGain = IXON_DEFAULT_GAIN;

	printSpeedInfo = false;

	addOption ('m', "mode", 1, "Which mode to use");
	addOption ('r', "root", 1, "directory with Andor detector.ini file");
	addOption ('g', "gain", 1, "set camera gain level (0-255)");
	addOption ('N', "noft", 0, "do not use frame transfer mode");
	addOption ('I', "speed_info", 0,
		"print speed info - information about speed available");
	addOption ('S', "ft-uses-shutter", 0, "force use of shutter with FT");
}


Rts2DevCameraAndor::~Rts2DevCameraAndor (void)
{
	ShutDown ();
}


void
Rts2DevCameraAndor::help ()
{
	std::cout << "Driver for Andor CCDs (iXon & others)" << std::endl;
	std::
		cout <<
		"Optimal values for vertical speed on iXon are: -H 1 -v 1 -C 1, those are default"
		<< std::endl;
	Rts2DevCamera::help ();
}


int
Rts2DevCameraAndor::setGain (int in_gain)
{
	int ret;
	if ((ret = SetEMCCDGain (in_gain)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setGain error " << ret << sendLog;
		return -1;
	}
	gain->setValueInteger (in_gain);
	return 0;
}


int
Rts2DevCameraAndor::setADChannel (int in_adchan)
{
	int ret;
	if ((ret = SetADChannel (in_adchan)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setADChannel error " << ret <<
			sendLog;
		return -1;
	}
	ADChannel->setValueInteger (in_adchan);
	return 0;
}


int
Rts2DevCameraAndor::setVSAmplitude (int in_vsamp)
{
	int ret;
	if ((ret = SetVSAmplitude (in_vsamp)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setVSAmplitude error " << ret <<
			sendLog;
		return -1;
	}
	VSAmp->setValueInteger (in_vsamp);
	return 0;
}


int
Rts2DevCameraAndor::setHSSpeed (int in_amp, int in_hsspeed)
{
	int ret;
	if ((ret = SetHSSpeed (in_amp, in_hsspeed)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setHSSpeed error " << ret <<
			sendLog;
		return -1;
	}
	EMOn->setValueBool (in_amp == 0 ? true : false);
	HSpeed->setValueInteger (in_hsspeed);
	return 0;

}


int
Rts2DevCameraAndor::setVSSpeed (int in_vsspeed)
{
	int ret;
	if ((ret = SetVSSpeed (in_vsspeed)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) << "andor setVSSpeed error " << ret <<
			sendLog;
		return -1;
	}
	VSpeed->setValueInteger (in_vsspeed);
	return 0;
}


int
Rts2DevCameraAndor::setFTShutter (int force)
{
	for (int i = 0; chips[i] != NULL; i++)
		((CameraAndorChip *) chips[i])->use_shutter_anyway = force;
	FTShutter->setValueBool ((force == 0) ? false : true);
	return 0;
}


int
Rts2DevCameraAndor::setMode (int in_mode)
{
	int ret;
	const ixon_mode *m;

	if ((in_mode < 0) || (in_mode >= IXON_MODES))
	{
		logStream (MESSAGE_ERROR) << "andor setMode failed: " << in_mode
			<< " is not a valid mode!" << ret << sendLog;
		return -1;
	}

	m = &mode_def[in_mode];
	if ((ret = setADChannel (m->ad)) != 0)
		return -1;

	if ((ret = setHSSpeed (m->disable_em, m->hsspeed)) != 0)
		return -1;

	if ((ret = setVSSpeed (m->vsspeed)) != 0)
		return -1;

	// only set VSAmp if it's defined
	if (VSAmp && (ret = setVSAmplitude (m->vs_amp)) != 0)
		return -1;

	// *FIXME*
	Mode->setValueInteger (in_mode);
	return 0;
}


void
Rts2DevCameraAndor::cancelPriorityOperations ()
{
	if (!isnan (defaultGain) && gain)
		setGain (defaultGain);
	Rts2DevCamera::cancelPriorityOperations ();
}


/********************************************************************
 * scriptEnds
 *
 * Ensure that we definitely leave the shutter closed.
 */

int
Rts2DevCameraAndor::scriptEnds ()
{
	if (!isnan (defaultGain) && gain)
		setGain (defaultGain);
	// *FIXME* Wow, this is ugly
	CameraAndorChip *c = (CameraAndorChip *) chips[0];
	c->closeShutter ();
	return Rts2DevCamera::scriptEnds ();
}


int
Rts2DevCameraAndor::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == gain)
		return setGain (new_value->getValueInteger ());
	if (old_value == Mode)
		return setMode (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == ADChannel)
		return setADChannel (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == VSAmp)
		return setVSAmplitude (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == EMOn)
		return setHSSpeed (((Rts2ValueBool *) new_value)->getValueBool ()? 0 : 1,
		HSpeed->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == HSpeed)
		return setHSSpeed (EMOn->getValueBool ()? 0 : 1,
		new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == VSpeed)
		return setVSSpeed (new_value->getValueInteger ()) == 0 ? 0 : -2;
	if (old_value == FTShutter)
		return setFTShutter (((Rts2ValueBool *) new_value)->getValueBool ()) ==
			0 ? 0 : -2;
	if (old_value == useFT)
	{
		CameraAndorChip *c = (CameraAndorChip *) chips[0];
		return c->setUseFT (((Rts2ValueBool *) new_value)->getValueBool ()) ==
			0 ? 0 : -2;
	}

	return Rts2DevCamera::setValue (old_value, new_value);
}


int
Rts2DevCameraAndor::processOption (int in_opt)
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
		case 'r':
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
			return Rts2DevCamera::processOption (in_opt);
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

void
Rts2DevCameraAndor::printCapabilities ()
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

int
Rts2DevCameraAndor::printNumberADCs ()
{
	int ret, n_ad;
	if ((ret = GetNumberADChannels (&n_ad)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) <<
			"andor cannot get number of AD channels" << sendLog;
		return -1;
	}
	printf ("AD Channels: %d (", n_ad);
	for (int ad = 0; ad < n_ad; ad++)
	{
		int depth;
		if ((ret = GetBitDepth (ad, &depth)) != DRV_SUCCESS)
		{
			logStream (MESSAGE_ERROR) <<
				"andor cannot get depth for ad " << ad << sendLog;
			return -1;
		}
		if (n_ad > 1)
			printf ("%d=%d-bit", ad, depth);
		else
			printf ("%d-bit", depth);
		if (ad == (n_ad - 1))
			printf (")\n");
		else
			printf (", ");
	}
	return n_ad;
}


int
Rts2DevCameraAndor::printHSSpeeds (int camera_type, int ad, int amp)
{
	int ret, nhs;
	if ((ret = GetNumberHSSpeeds (ad, amp, &nhs)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) <<
			"andor cannot get number of horizontal speeds " << sendLog;
		return -1;
	}

	printf ("Horizontal speeds: %d (", nhs);

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
		printf ("%.2f", val);
		if (s == (nhs - 1))
			switch (camera_type)
			{
				case AC_CAMERATYPE_IXON:
					printf (" MHz)\n");
					break;
				default:
					printf (" usec/pix)\n");
					break;
			}
			else
				printf (", ");
	}
	return 0;
}


int
Rts2DevCameraAndor::printVSSpeeds ()
{
	int ret, vspeeds;
	if ((ret = GetNumberVSSpeeds (&vspeeds)) != DRV_SUCCESS)
	{
		logStream (MESSAGE_ERROR) <<
			"andor init cannot get vertical speeds" << sendLog;
		return -1;
	}
	printf ("Vertical Speeds: %d (", vspeeds);
	for (int s = 0; s < vspeeds; s++)
	{
		float val;
		GetVSSpeed (s, &val);
		printf ("%.2f", val);
		if (s == (vspeeds - 1))
			printf (" usec/pix)\n");
		else
			printf (", ");
	}
	return 0;
}


int
Rts2DevCameraAndor::printInfo ()
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
			printf ("<unknown> (code is %lu)", cap.ulCameraType);
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
			if ((ret = printHSSpeeds (cap.ulCameraType, ad, amp)) != 0)
				return ret;

	if ((ret = printVSSpeeds ()) != 0)
		return ret;

	return 0;
}


int
Rts2DevCameraAndor::init ()
{
	CameraAndorChip *cc;
	unsigned long err;
	int ret;

	if ((ret = Rts2DevCamera::init ()) != 0)
		return ret;

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

	initAndorValues ();

	//Set Read Mode to --Image--
	ret = SetReadMode (4);
	if (ret != DRV_SUCCESS)
	{
		cerr << "Cannot set read mode (" << ret << "), exiting" << endl;
		return -1;
	}

	if (setMode (0) != 0)
		return -1;

	chipNum = 1;

	// *FIXME* this is borked until we figure out exactly who knows about
	// shutters vs. FT - hardcoded as "gain=1", "use ft=1"
	cc = new CameraAndorChip (this, 0, 1, useFT->getValueBool ());
	chips[0] = cc;
	chips[1] = NULL;

	if (printSpeedInfo)
		printInfo ();

	sprintf (ccdType, "ANDOR");

	return Rts2DevCamera::initChips ();
}


void
Rts2DevCameraAndor::initAndorValues ()
{
	if (cap.ulCameraType == AC_CAMERATYPE_IXON)
	{
		createValue (VSAmp, "SAMPLI", "Used andor shift amplitude", true, 0,
			CAM_EXPOSING | CAM_READING | CAM_DATA, true);
		VSAmp->setValueInteger (0);
	}
	if (cap.ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN)
	{
		createValue (gain, "GAIN", "CCD gain", true, 0,
			CAM_EXPOSING | CAM_READING | CAM_DATA, true);
		setGain (defaultGain);
	}
}


int
Rts2DevCameraAndor::ready ()
{
	return 0;
}


void
Rts2DevCameraAndor::getTemp ()
{
	int c_status;
	float tmpTemp;
	c_status = GetTemperatureF (&tmpTemp);
	if (!
		(c_status == DRV_ACQUIRING || c_status == DRV_NOT_INITIALIZED
		|| c_status == DRV_ERROR_ACK))
	{
		tempCCD->setValueDouble (tmpTemp);
		tempRegulation->setValueInteger (c_status != DRV_TEMPERATURE_OFF);
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "andor info status " << c_status <<
			sendLog;
	}
}


int
Rts2DevCameraAndor::info ()
{
	if (isIdle ())
	{
		getTemp ();
	}
	return Rts2DevCamera::info ();
}


int
Rts2DevCameraAndor::camChipInfo (int chip)
{
	return 0;
}


int
Rts2DevCameraAndor::camCoolMax ()
{
	return camCoolHold ();
}


int
Rts2DevCameraAndor::camCoolHold ()
{
	if (isnan (nightCoolTemp))
		return camCoolTemp (-5);
	else
		return camCoolTemp (nightCoolTemp);
}


int
Rts2DevCameraAndor::camCoolTemp (float new_temp)
{
	CoolerON ();
	SetTemperature ((int) new_temp);
	tempSet->setValueDouble (new_temp);
	return 0;
}


int
Rts2DevCameraAndor::camCoolShutdown ()
{
	CoolerOFF ();
	SetTemperature (20);
	tempSet->setValueDouble (+50);
	// *FIXME* Wow, this is ugly
	CameraAndorChip *c = (CameraAndorChip *) chips[0];
	c->closeShutter ();
	return 0;
}


int
Rts2DevCameraAndor::camExpose (int chip, int light, float exptime)
{
	getTemp ();
	return Rts2DevCamera::camExpose (chip, light, exptime);
}


int
main (int argc, char **argv)
{
	Rts2DevCameraAndor device = Rts2DevCameraAndor (argc, argv);
	return device.run ();
}
