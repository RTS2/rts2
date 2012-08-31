/* 
 * Astronomical Research Camera driver
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include <config.h>

#include "camd.h"

#ifdef ARC_API_1_7
#include "Driver.h"
#include "DSPCommand.h"
#include "LoadDspFile.h"
#include "Memory.h"
#include "ErrorString.h"

#include <sys/ioctl.h>

#define HARDWARE_DATA_MAX	1000000

#else
#include "CController/CController.h"
#include "CDeinterlace/CDeinterlace.h"
#endif

#define OPT_TIM     OPT_LOCAL + 42
#define OPT_UTIL    OPT_LOCAL + 43

#define OPT_WIDTH   OPT_LOCAL + 44
#define OPT_HEIGHT  OPT_LOCAL + 45

namespace rts2camd
{

/**
 * Class for Astronomical Research Cameras (Leech controller) drivers.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Arc:public Camera
{
	public:
		Arc (int argc, char **argv);
		virtual ~Arc ();

		virtual int info ();
		virtual int scriptEnds ();
		virtual int killAll (bool callScriptEnds);

	protected:
		int processOption (int opt);
		int init ();

		virtual void initBinnings ();
		virtual int setBinning (int in_vert, int in_hor);

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

#ifdef ARC_API_1_7

#else
		virtual int setCoolTemp (float new_temp);
#endif

		virtual int startExposure ();
		virtual long isExposing ();
		virtual int stopExposure ();
		virtual int doReadout ();

	private:
		int w;
		int h;

		rts2core::ValueBool *power;
		rts2core::ValueInteger *biasWidth;
		rts2core::ValueInteger *biasPosition;

		rts2core::ValueString *timFile;
                rts2core::ValueString *utilFile;

#ifdef ARC_API_1_7
		HANDLE pci_fd;
		unsigned short *mem_fd;

		int num_pci_tests;
		int num_tim_tests;
		int num_util_tests;

		bool validate;

		int configWord;

		int lastPixelCount;

		int do_controller_setup ();
		int shutter_position ();
#else
		arc::CController controller;
		long lDeviceNumber;
		rts2core::ValueBool *synthetic;
		
		// if chip should be clear during exposure..
		rts2core::ValueSelection *clearCCD;
#endif
};

}

using namespace rts2camd;

Arc::Arc (int argc, char **argv):Camera (argc, argv)
{
#ifdef ARC_API_1_7
	num_pci_tests = 1055;
	num_tim_tests = 1055;
	num_util_tests = 10;
#else
	lDeviceNumber = 0;

	createTempSet ();
#endif

	createExpType ();
	createTempCCD ();

	createValue (power, "power", "controller power", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	createValue (biasWidth, "bias_width", "Width of bias", true, RTS2_VALUE_WRITABLE);
	biasWidth->setValueInteger (0);
	createValue (biasPosition, "bias_position", "Position of bias", true, RTS2_VALUE_WRITABLE);
	biasPosition->setValueInteger (0);

#ifdef ARC_API_1_7

#else
	createValue (clearCCD, "CLEAR", "clear CCD before exposure", false, RTS2_VALUE_WRITABLE);
	clearCCD->addSelVal ("normal");
	clearCCD->addSelVal ("first in sequence");
	clearCCD->addSelVal ("in sequence");
	clearCCD->addSelVal ("last in sequence");
	clearCCD->setValueInteger (0);

	createValue (synthetic, "synthetic", "use synthetic image", true, RTS2_VALUE_WRITABLE);
	synthetic->setValueBool (false);
#endif
	createValue (timFile, "time-file", "DSP timing file", false);
        createValue (utilFile, "util-file", "utility file", false);

#ifdef ARC_API_1_7

#else
	addOption ('n', NULL, 1, "Device number (default 0)");
#endif
	addOption (OPT_WIDTH, "width", 1, "chip width - number of collumns");
	addOption (OPT_HEIGHT, "height", 1, "chip height - number of rows/lines");
	addOption (OPT_TIM, "time-file", 1, "timing file");
        addOption (OPT_UTIL, "utility-file", 1, "utility file");

	w = 1024;
	h = 1024;
}

Arc::~Arc ()
{
#ifdef ARC_API_1_7
	closeDriver (pci_fd);
	free_memory (pci_fd, mem_fd);
#else
	controller.CloseDriver ();
#endif
}

int Arc::info ()
{
#ifdef ARC_API_1_7

#else
	try
	{
		if (getState () & CAM_WORKING)
			return 0;
		tempCCD->setValueDouble (controller.GetArrayTemperature ());
	}
	catch (std::exception ex)
	{
		logStream (MESSAGE_ERROR) << "Failure while retrieving informations" << sendLog;
		return -1;
	}
#endif
	return Camera::info ();
}

int Arc::scriptEnds ()
{
	clearCCD->setValueInteger (0);
	return Camera::scriptEnds ();
}

int Arc::killAll (bool callScriptEnds)
{
#ifdef ARC_API_1_7

#else
	if (getState () & CAM_READING)
	{
		try
		{
			controller.PCICommand (ABORT_READOUT);
			sleep (2);
			controller.PCICommand (PCI_RESET);
			maskState (CAM_MASK_READING, 0, "reading interrupted");
		}
		catch (std::exception ex)
		{
			logStream (MESSAGE_ERROR) << "kill with ex" << ex.what () << sendLog;
		}
	}
	// reset controller
#endif
	// readout must end..
	while (controller.IsReadout ())
	{
		sleep (1);
                //long lReply = controller.Command (arc::TIM_ID, 0x0202);
                //controller.CheckReply (lReply);
	}
	return Camera::killAll (callScriptEnds);
}

int Arc::processOption (int opt)
{
	switch (opt)
	{
#ifdef ARC_API_1_7

#else
		case 'n':
			lDeviceNumber = atoi (optarg);
			break;
#endif
		case OPT_WIDTH:
			w = atoi (optarg);
			break;
	        case OPT_HEIGHT:
			h = atoi (optarg);
			break;
		case OPT_TIM:
			timFile->setValueCharArr (optarg);
			break;
                case OPT_UTIL:
                        utilFile->setValueCharArr (optarg);
                        break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int Arc::init ()
{
	int ret = Camera::init ();
	if (ret)
		return ret;
#ifdef ARC_API_1_7
	/* Open the device driver */
	pci_fd = openDriver("/dev/astropci0");

	int bufferSize = (w + 20) * (h + 20) * 2;
	if ((mem_fd = create_memory(pci_fd, w, h, bufferSize)) == NULL) {
		logStream (MESSAGE_ERROR) << "Unable to create image buffer: 0x" << mem_fd << " (0x" << std::hex << mem_fd << ")%" << sendLog;
		return -1;
	}

	setSize (w, h, 0, 0);

	return do_controller_setup ();
#else
	try
	{
		controller.GetDeviceBindings ();
		controller.OpenDriver (0, h * w * sizeof (unsigned int16_t));
	        controller.SetupController( true,          // Reset Controller
	                                     true,          // Test Data Link ( TDL )
	                                     true,          // Power On
	                                     h,      // Image row size
	                                     w,      // Image col size
	                                     timFile->getValue ());    // DSP timing file
		if (utilFile->getValue ())
		{
			logStream (MESSAGE_DEBUG) << "loading " << utilFile->getValue () << sendLog;
			controller.LoadControllerFile (utilFile->getValue ());
		}
		power->setValueBool (true);
		setSize (controller.GetImageCols (), controller.GetImageRows (), 0, 0);
		long lReply = controller.Command (arc::TIM_ID, SOS, AMP_0);
		controller.CheckReply (lReply);
	}
	catch (std::runtime_error &ex)
	{
		logStream (MESSAGE_ERROR) << "Failed to initialize camera:" << ex.what () << sendLog;
		return -1;
	}
	catch (...)
	{
		logStream (MESSAGE_ERROR) << "Error: unknown exception occurred!!" << sendLog;
		controller.CloseDriver ();
		return -1;
	}
	return 0;
#endif
}

void Arc::initBinnings ()
{
	Camera::initBinnings ();

	addBinning2D (2, 2);
	addBinning2D (3, 3);
	addBinning2D (4, 4);
	addBinning2D (1, 2);
	addBinning2D (2, 1);
	addBinning2D (4, 1);
}

int Arc::setBinning (int in_vert, int in_hori)
{
	try
	{
		int o_v, o_h;
		controller.SetBinning (getHeight (), getWidth (), in_vert, in_hori, &o_v, &o_h);
		logStream (MESSAGE_DEBUG) << "set binning " << in_vert << "x" << in_hori << " with outputs " << o_v << "x" << o_h << sendLog;
		return Camera::setBinning (in_vert, in_hori);
	}
	catch (std::exception er)
	{
		return -1;
	}
}

int Arc::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
#ifdef ARC_API_1_7
	if (old_value == power)
	{
		doCommand (pci_f, TIM_ID, (rts2core::ValueBool *) (new_value->getValueInteger ()) ? PON : POF, DON);
		return 0;
	}
#else
	if (old_value == synthetic)
	{
		controller.SetSyntheticImageMode (((rts2core::ValueBool *) new_value)->getValueBool ());
		return 0;
	}
	else if (old_value == power)
	{
		controller.Command (arc::TIM_ID, (rts2core::ValueBool *) (new_value->getValueInteger ()) ? PON : POF);
		return 0;
	}
#endif
	return Camera::setValue (old_value, new_value);
}

#ifdef ARC_API_1_7

#else
int Arc::setCoolTemp (float new_temp)
{
	controller.SetArrayTemperature (new_temp);
	return Camera::setCoolTemp (new_temp);
}
#endif

int Arc::startExposure ()
{
#ifdef ARC_API_1_7
  	// set readout area..
	if (chipUsedReadout->wasChanged ())
	{
		int biasOffset = getWidth () - (chipUsedReadout->getXInt () + chipUsedReadout->getWidthInt ()) ; //biasPosition->getValueInteger () - subImageCenterCol - subImageWidth/2;
		int data = 1;
		
		for (int i = 0; i < 10; i++)
		{
			if (doCommand1 (pci_fd, TIM_ID, TDL, data, data) == _ERROR)
			{
				if (getError () != TOUT)
				{
					logStream (MESSAGE_ERROR) <<  "ERROR doing timing hardware tests: 0x" << std::hex << getError () << sendLog;
					return -1;
				}
				usleep (USEC_SEC / 50);
			}
			else
			{
				break;
			}
		}
	
		// Set the new image dimensions
		logStream (MESSAGE_DEBUG) << "Updating image columns " << chipUsedReadout->getWidthInt () << ", rows " << chipUsedReadout->getHeightInt () << ", biasWidth " << biasWidth->getValueInteger () << sendLog;

		if (doCommand2 (pci_fd, TIM_ID, WRM, (Y | 1), chipUsedReadout->getWidthInt () + biasWidth->getValueInteger (), DON) == _ERROR)
		{
			logStream (MESSAGE_ERROR) << "Failed to set image columns -> 0x" << std::hex << getError() << sendLog;
			return -1;
		}

		if (doCommand2 (pci_fd, TIM_ID, WRM, (Y | 2), chipUsedReadout->getHeightInt (), DON) == _ERROR)
		{
			logStream (MESSAGE_ERROR) << "Failed to set image rows -> 0x" << std::hex << getError() << sendLog;
			return -1;
		}

		if (doCommand3 (pci_fd, TIM_ID, SSS, biasWidth->getValueInteger (), chipUsedReadout->getWidthInt (), chipUsedReadout->getHeightInt (), DON) == _ERROR)
		{
			logStream (MESSAGE_ERROR) << "Failed to set image bias width etc.. -> 0x" << std::hex << getError() << sendLog;
			return -1;
		}

		//if (doCommand3 (pci_fd, TIM_ID, SSP, chipUsedReadout->getXInt (), chipUsedReadout->getYInt (), biasOffset, DON) == _ERROR)
		//{
		//	logStream (MESSAGE_ERROR) << "Failed to send sub-array POSITION info -> 0x" << std::hex << getError() << sendLog;
		//	return -1;
		//}
	}

	if (doCommand (pci_fd, TIM_ID, CLR, DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "cannot perform fast clear before exposure: 0x" << std::hex << getError () << sendLog;
		return -1;
	}

	if (shutter_position () == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "ERROR: Setting shutter position failed: 0x" << std::hex << getError () << sendLog;
		return -1;
	}

	if (doCommand1 (pci_fd, TIM_ID, SET, getExposure () * 1000.0, DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "ERROR: Setting exposure time failed: 0x" << std::hex << getError () << sendLog;
		return -1;
	}

	if (doCommand (pci_fd, TIM_ID, SEX, DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "ERROR: Starting exposure failed -> 0x" << std::hex << getError () << sendLog;
		return -1;
	}

	lastPixelCount = 0;

	return 0;
#else
	try
	{
		long lReply;
		if (chipUsedReadout->wasChanged ())
		{
			int bw = chipUsedReadout->getWidthInt () / binningHorizontal ();
			int bh = chipUsedReadout->getHeightInt () / binningVertical ();
			int x = chipUsedReadout->getXInt () + bw / 2;
			int y = chipUsedReadout->getYInt () + bh / 2;
			int oRows, oCols;
			controller.SetSubArray (oRows, oCols, y, x, bh, bw, getWidth (), 0);
		}
		if (clearCCD->getValueInteger ())
		{
			if (clearCCD->getValueInteger () == 1)
				controller.Command (arc::TIM_ID, CLR);
			controller.Command (arc::TIM_ID, OSH);
			return 1;
		}
		lReply = controller.Command (arc::TIM_ID, SET, (long) (getExposure () * 1000));
		controller.CheckReply (lReply);
		controller.SetOpenShutter (getExpType () == 0);
		// start exposure..
		lReply = controller.Command (arc::TIM_ID, SEX);
		controller.CheckReply (lReply);
			
		return 0;
	}
	catch (std::runtime_error &er)
	{
		return -1;
	}
#endif
}

long Arc::isExposing ()
{
#ifdef ARC_API_1_7
	int status = (getHstr (pci_fd) & HTF_BITS) >> 3;

	if (status != READOUT)
		return Camera::isExposing ();

	return -2;
#else
	if (clearCCD->getValueInteger ())
	{
		int ret = Camera::isExposing ();
		if (ret == -2)
		{
			controller.Command (arc::TIM_ID, CSH);
			return (clearCCD->getValueInteger () == 3) ? -2 : -4;
		}
		return ret;
	}
	try
	{
		long lPixCnt = controller.GetPixelCount ();
		if (!controller.IsReadout () && lPixCnt == 0)
		{
			long lReply = controller.Command (arc::TIM_ID, RET);
			return USEC_SEC * (getExposure () - lReply / 1000.0);
		}
	}
	catch (std::runtime_error &er)
	{
		logStream (MESSAGE_ERROR) << "error in isExposing " << er.what () << sendLog;
		return -1;
	}
	return -2;
#endif
}

int Arc::stopExposure ()
{
#ifdef ARC_API_1_7
	if (doCommand (pci_fd, TIM_ID, PEX, DON) == _ERROR)
		logStream (MESSAGE_ERROR) << "cannot stop exposure" << sendLog;
#else
	try
	{
		controller.Command (arc::TIM_ID, PEX);
		controller.PCICommand (PCI_RESET);
	}
	catch (std::runtime_error &er)
	{
		logStream (MESSAGE_ERROR) << "error in stopExposure " << er.what () << sendLog;
		return -1;	
	}
#endif
	return Camera::stopExposure ();
}

int Arc::doReadout ()
{
#ifdef ARC_API_1_7
	int currentPixelCount;
	ioctl (pci_fd, ASTROPCI_GET_PROGRESS, &currentPixelCount);

	if (currentPixelCount > lastPixelCount)
	{
		sendReadoutData ((char *) (mem_fd + lastPixelCount), (currentPixelCount - lastPixelCount) * 2);
		if (currentPixelCount == chipUsedSize ())
			return -2;
		lastPixelCount = currentPixelCount;
	}
	return USEC_SEC / 8.0;
#else
	try
	{
		if (controller.IsReadout ())
			return USEC_SEC / 1000.0;
		if (controller.GetPixelCount () < chipUsedSize ())
			return USEC_SEC / 1000.0;

		arc::CDeinterlace deint;
		deint.RunAlg (controller.mapFd, getUsedHeight (), getUsedWidth (), arc::CDeinterlace::DEINTERLACE_NONE);
		sendReadoutData ((char *) controller.mapFd, chipUsedSize () * 2);
		return -2;
	}
	catch (std::runtime_error &er)
	{
		logStream (MESSAGE_ERROR) << "error in doReadout " << er.what () << sendLog;
		return -1;	
	}
#endif
}

#ifdef ARC_API_1_7
int Arc::do_controller_setup ()
{
	int data = 1;
	int i = 0;

	/* PCI TESTS */
	logStream (MESSAGE_DEBUG) << "Doing " << num_pci_tests << " PCI hardware tests." << sendLog;
	for (i = 0; i < num_pci_tests; i++)
	{
		if (doCommand1 (pci_fd, PCI_ID, TDL, data, data) == _ERROR)
			logStream (MESSAGE_ERROR) << "ERROR doing PCI hardware tests: 0x" << std::hex <<  getError () << sendLog;
		data += HARDWARE_DATA_MAX / num_pci_tests;
	}

	logStream (MESSAGE_DEBUG) << "Doing " << num_tim_tests << " timing hardware tests." << sendLog;
	for (i = 0; i < num_tim_tests; i++)
	{
		if (doCommand1 (pci_fd, TIM_ID, TDL, data, data) == _ERROR)
			logStream (MESSAGE_ERROR) <<  "ERROR doing timing hardware tests: 0x" << std::hex << getError () << sendLog;
		data += HARDWARE_DATA_MAX / num_tim_tests;
	}

	logStream (MESSAGE_DEBUG) << "Doing " << num_util_tests <<  " utility hardware tests." << sendLog;
	for (i = 0; i < num_util_tests; i++)
	{
		if (doCommand1 (pci_fd, UTIL_ID, TDL, data, data) == _ERROR)
			logStream (MESSAGE_ERROR) << "ERROR doing utility hardware tests: 0x" << std::hex << getError() << sendLog;
		data += HARDWARE_DATA_MAX / num_util_tests;
	}

	logStream (MESSAGE_DEBUG) << "Resetting controller" << sendLog;
	if (hcvr (pci_fd, RESET_CONTROLLER, UNDEFINED, SYR) == _ERROR)
  	{
		logStream (MESSAGE_ERROR) << "Failed resetting controller. Could not reset controller." << sendLog;
		return -1;
	}

	/*logStream (MESSAGE_DEBUG) << "Stopping camera idle" << sendLog;
	if (doCommand (pci_fd, TIM_ID, STP, DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "cannot stop idle mode" << sendLog;
		return -1;
	}*/

	/* -----------------------------------
	   LOAD TIMING FILE/APPLICATION
	   ----------------------------------- */
	if (strlen (timFile->getValue ()))
	{
		logStream (MESSAGE_DEBUG) << "Loading timing file " << timFile->getValue () << sendLog;
		loadFile (pci_fd, (char *) (timFile->getValue ()), validate);
		if (!validate)
			throw rts2core::Error ("cannot load timing file");
		logStream (MESSAGE_DEBUG) << "Timing file loaded" << sendLog;
	}

	if (strlen (utilFile->getValue ()))
	{
		logStream (MESSAGE_DEBUG) << "Loading utilitty file " << utilFile->getValue () << sendLog;
		loadFile (pci_fd, (char *) (utilFile->getValue ()), validate);
		if (!validate)
			throw rts2core::Error ("cannot load utility file");
		logStream (MESSAGE_DEBUG) << "Utility file loaded" << sendLog;
	}


	logStream (MESSAGE_DEBUG) << "Doing power on." << sendLog;
	if (doCommand (pci_fd, TIM_ID, PON, DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "Power on failed -> 0x" << std::hex << getError() << sendLog;
		return -1;
	}
	power->setValueBool (true);

	/* -----------------------------------
	   SET DIMENSIONS
	   ----------------------------------- */
	logStream (MESSAGE_DEBUG) << "Setting image columns " << getUsedWidth () << sendLog;
	if (doCommand2 (pci_fd, TIM_ID, WRM, (Y | 1), getUsedWidth (), DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "Failed setting image collumns -> 0x" << std::hex << getError() << sendLog;
		return -1;
	}

	logStream (MESSAGE_DEBUG) << "Setting image rows " << getUsedHeight () << sendLog;
	if (doCommand2 (pci_fd, TIM_ID, WRM, (Y | 2), getUsedHeight (), DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "Failed setting image rows -> 0x" << std::hex << getError() << sendLog;
		return -1;
	}

	logStream (MESSAGE_DEBUG) << "setting amplifiers" << sendLog;
	if (doCommand1 (pci_fd, TIM_ID, SOS, R_AMP, DON) == _ERROR)
	{
		logStream (MESSAGE_ERROR) << "cannot set right-only amplifier" << sendLog;
		return -1;    
	}

	/* --------------------------------------
	   READ THE CONTROLLER PARAMETERS INFO
	   -------------------------------------- */
	configWord = doCommand (pci_fd, TIM_ID, RCC, UNDEFINED);

	return 0;
}

int Arc::shutter_position ()
{
	int currentStatus = doCommand1(pci_fd, TIM_ID, RDM, (X | 0), UNDEFINED);

	if (getExpType () == 0)
	{
		if (doCommand2(pci_fd, TIM_ID, WRM, (X | 0), (currentStatus | _OPEN_SHUTTER_), DON) == _ERROR)
			return _ERROR;
	}			
	else
	{
		if (doCommand2(pci_fd, TIM_ID, WRM, (X | 0), (currentStatus & _CLOSE_SHUTTER_), DON) == _ERROR)
			return _ERROR;
	}

	return _NO_ERROR;
}
#endif

int main (int argc, char **argv)
{
	Arc device (argc, argv);
	return device.run ();
}
