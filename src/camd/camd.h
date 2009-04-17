/* 
 * Basic camera daemon
 * Copyright (C) 2001-2007 Petr Kubanek <petr@kubanek.net>
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

/**
 * @file Abstract camera class.
 *
 * @defgroup RTS2Camera Camera driver
 */

#ifndef __RTS2_CAMERA_CPP__
#define __RTS2_CAMERA_CPP__

#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2scriptdevice.h"
#include "imghdr.h"

#include "filter.h"

#define MAX_CHIPS  3
#define MAX_DATA_RETRY 100

namespace rts2camd
{

/**
 * Class which holds 2D binning informations.
 *
 * @ingroup RTS2Camera
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Binning2D: public Rts2SelData
{
	public:
		/**
		 * Vertical binning factor.
		 */
		int vertical;

		/**
		 * Horizontal binnning factor.
		 */
		int horizontal;

		Binning2D (int in_vertical, int in_horizontal): Rts2SelData ()
		{
			vertical = in_vertical;
			horizontal = in_horizontal;
		}

		/**
		 * Return string description of binning.
		 *
		 * @return String describing binning in standard <vertical> x <horizontal> form.
		 */
		std::string getDescription ()
		{
			std::ostringstream _os;
			_os << vertical << "x" << horizontal;
			return _os.str();
		}
};

/**
 * Holds type of data produced.
 *
 * @ingroup RTS2Camera
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataType: public Rts2SelData
{
	public:
		int type;

		DataType (int in_type): Rts2SelData ()
		{
			type = in_type;
		}
};

/**
 * Abstract class for camera device.
 *
 * During complete exposure cycle (EXPOSURE-DATA-READOUT), all values which
 * have connection to chip size etc. does not change, so you can safely use
 * them during exposure cycle. The changes are put to que and executed once
 * camera enters IDLE cycle.
 *
 * Folowing state diagram depict possible state transation inside
 * Camera.
 *
 * @dot
digraph "Camera states" {
	idle [shape=box];
	exposing [shape=box];
	"exposure end" [shape=diamond];
	"readout end" [shape=diamond];
	readout [shape=box];
	"readout | exposing" [shape=box];

	{ rank = same; idle; exposing; "exposure end"}
	{ rank = same; "readout | exposing" ; readout}

	idle -> exposing [label="expose"];
	exposing -> "exposure end" [label="exposure ended"];
	"exposure end" -> "readout | exposing" [label="exposure_number>0\nsupportFT\nqueValues.empty"];

	"readout | exposing" -> exposing [label="readout end"];
	"readout | exposing" -> readout [label="exposure end"];
	"exposure end" -> readout;
	readout -> "readout end";
	"readout end" -> idle [label="exposure_number==0"];
	"readout end" -> exposing [label="exposure_number>0"];
}
 * @enddot
 *
 * @ingroup RTS2Camera
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Camera:public Rts2ScriptDevice
{
	private:
		// comes from CameraChip
		void initData ();

		time_t readout_started;

		// connection which requries data to be send after end of exposure
		Rts2Conn *exposureConn;

		// number of exposures camera takes
		Rts2ValueLong *exposureNumber;
		Rts2ValueBool *waitingForEmptyQue;
		Rts2ValueBool *waitingForNotBop;

		char *focuserDevice;
		char *wheelDevice;

		int lastFilterNum;

		int currentImageData;
								 // DARK of LIGHT frames
		Rts2ValueFloat *exposure;

		Rts2ValueRectangle *chipSize;

		int camStartExposure ();
		int camStartExposureWithoutCheck ();

		// when we call that function, we must be sure that either filter or wheelDevice != NULL
		int camFilter (int new_filter);

		Rts2ValueSelection *camFilterVal;
		Rts2ValueInteger *camFocVal;
		Rts2ValueDouble *rotang;

		int getStateChip (int chip);

		// chip binning
		Rts2ValueSelection *binning;

		// allowed chip data type
		Rts2ValueSelection *dataType;

		// when chip exposure will end
		Rts2ValueTime *exposureEnd;

		// set chipUsedSize size
		int box (int in_x, int in_y, int in_width, int in_height);

		// callback functions from camera connection
		int camExpose (Rts2Conn * conn, int chipState, bool fromQue);
		int camBox (Rts2Conn * conn, int x, int y, int width, int height);
		int camCenter (Rts2Conn * conn, int in_w, int in_h);

		int sendFirstLine ();

		// if true, send command OK after exposure is started
		bool sendOkInExposure;

	protected:
		// comes from CameraChip
		double pixelX;
		double pixelY;

		// buffer used to read data
		char* dataBuffer;
		long dataBufferSize;

		/**
		 * Return exected exposure end.
		 *
		 * @return Expected exposure end (ctime in with usec).
		 */
		double getExposureEnd ()
		{
			return exposureEnd->getValueDouble ();
		}

		Rts2ValueDouble *subExposure;

		//! number of connection waiting to be executed
		Rts2ValueInteger *quedExpNumber;

		/**
		 * Shutter control.
		 */
		Rts2ValueSelection *expType;

		/**
		 * Change state of camera chip.
		 *
		 * This function changes state of chip. chip_state_mask and chip_new_state are
		 * shifted according to chip_num value, so they should contain common state
		 * mask and values, as defined in status.h. state_mask and new_state are not shifted.
		 *
		 * @param chip_state_mask  Chip state mask.
		 * @param chip_new_state New chip state.
		 * @param state_mask State mask for whole device. This argument should contain various BOP_XXX values.
		 * @param new_state New state for whole device.
		 * @param description Text describing operation which is performed.
		 */
		void maskStateChip (int chip, int chip_state_mask, int chip_new_state,
			int state_mask, int new_state, const char *description);

		/**
		 * Returns number of exposure camera is currently taking or has taken from camera startup.
		 *
		 * @return Exposure number.
		 */
		long getExposureNumber ()
		{
			return exposureNumber->getValueLong ();
		}

		/**
		 * Increment exposure number. Must be called when new exposure started by "itself".
		 */
		void incExposureNumber ()
		{
			exposureNumber->inc ();
			sendValueAll (exposureNumber);
		}

		const int getDataType ()
		{
			return ((DataType *) dataType->getData ())->type;
		}

		int nAcc;
		struct imghdr focusingHeader;

		/**
		 * Send whole image, including header.
		 *
		 * @param data Data to send.
		 * @param dataSize Size of data which will be send.
		 *
		 * @return -1 on error, 0 otherwise.
		 */
		int sendImage (char *data, size_t dataSize);

		int sendReadoutData (char *data, size_t dataSize);
		long getWriteBinaryDataSize ()
		{
			if (exposureConn)
				return exposureConn->getWriteBinaryDataSize (currentImageData);
			return 0;
		}

		void addBinning2D (int bin_v, int bin_h);

		/**
		 * Defines possible binning values.
		 */
		virtual void initBinnings ();

		void addDataType (int in_type);

		/**
		 * Defines data type values.
		 */
		virtual void initDataTypes ();

		/**
		 * Return vertical binning.
		 */
		int binningHorizontal ()
		{
			return ((Binning2D *)(binning->getData ()))->horizontal;
		}

		/**
		 * Return vertical binning.
		 */
		int binningVertical ()
		{
			return ((Binning2D *)(binning->getData ()))->vertical;
		}

		/**
		 * Get size of pixel in bytes.
		 *
		 * @return Size of pixel in bytes.
		 */
		const int usedPixelByteSize ()
		{
			if (getDataType () == RTS2_DATA_ULONG)
				return 4;
			return abs (getDataType () / 8);
		}

		/**
		 * Returns chip size in pixels. This method must be overloaded
		 * when camera decides to use non-standart 2D horizontal and
		 * vertical binning.
		 *
		 * @return Chip size in pixels.
		 */
		virtual long chipUsedSize ()
		{
			return getUsedWidthBinned () * getUsedHeightBinned ();
		}

		/**
		 * Retuns size of chip in bytes.
		 *
		 * @return Size of pixel data from current configured CCD in bytes.
		 */
		virtual long chipByteSize ()
		{
			return chipUsedSize () * usedPixelByteSize ();
		}

		/**
		 * Returns size of one line in bytes.
		 */
		int lineByteSize ()
		{
			return usedPixelByteSize () * (chipUsedReadout->getWidthInt ());
		}

		virtual int processData (char *data, size_t size);

		Rts2ValueRectangle *chipUsedReadout;

		/**
		 * Handles options passed on the command line. For a list of options, please
		 * see output of component with -h parameter passed on command line.
		 *
		 * @param in_opt  Option.
		 *
		 * @return 0 when option was processed sucessfully, otherwise -1.
		 */
		virtual int processOption (int in_opt);

		int willConnect (Rts2Address * in_addr);
		char *device_file;
		// temperature and others; all in deg Celsius
		Rts2ValueFloat *tempAir;
		Rts2ValueFloat *tempCCD;
		Rts2ValueFloat *tempSet;

		// night cooling temperature
		Rts2ValueFloat *nightCoolTemp;

		char ccdType[64];
		char *ccdRealType;
		char serialNumber[64];

		Filter *filter;

		virtual void checkQueChanges (int fakeState);

		virtual void cancelPriorityOperations ();

		Rts2ValueDouble *rnoise;

		void initCameraChip ();
		void initCameraChip (int in_width, int in_height, double in_pixelX, double in_pixelY);

		virtual int startExposure () = 0;
		virtual void afterReadout ();

		virtual int endReadout ();

		virtual int readoutOneLine () = 0;
		void clearReadout ();

		void setSize (int in_width, int in_height, int in_x, int in_y)
		{
			chipSize->setInts (in_x, in_y, in_width, in_height);
			chipUsedReadout->setInts (0, 0, in_width, in_height);
		}

		/**
		 * Suggest size of data buffer.  Data buffer is used to readout
		 * and then send the data. It should be big enought to hold
		 * largest element which is possible to get from camera driver.
		 * Default return full chip size, but can be overwritten in
		 * descendants.
		 *
		 * @return Size of data buffer in bytes.
		 */
		virtual long suggestBufferSize ()
		{
			return chipByteSize ();
		}

		/**
		 * Get chip width (in pixels).
		 *
		 * @return Chip width in pixels.
		 */
		const int getWidth ()
		{
			return chipSize->getWidthInt ();
		}

		/**
		 * Get used area width (in pixels).
		 *
		 * @return Used area width in pixels.
		 */
		const int getUsedWidth ()
		{
			return chipUsedReadout->getWidthInt ();
		}

		/**
		 * Get width of used area divided by binning factor.
		 *
		 * @return getUsedWidth() / binningHorizontal()
		 */
		const int getUsedWidthBinned ()
		{
			return (int) (ceil (getUsedWidth () / binningHorizontal ()));
		}

		/**
		 * Returns size of single row in bytes.
		 *
		 * @return getUsedWidthBinned() * usedPixelByteSize()
		 */
		const int getUsedRowBytes ()
		{
			return getUsedWidthBinned () * usedPixelByteSize ();
		}

		/**
		 * Get chip width (in pixels).
		 *
		 * @return Chip width in pixels.
		 */
		const int getHeight ()
		{
			return chipSize->getHeightInt ();
		}

		/**
		 * Get X offset of used aread (in pixels)
		 *
		 */
		const int getUsedY ()
		{
			return chipUsedReadout->getYInt ();
		}

		/**
		 * Get X offset of used aread (in pixels)
		 *
		 */
		const int getUsedX ()
		{
			return chipUsedReadout->getXInt ();
		}

		/**
		 * Get width of used area (in pixels).
		 *
		 * @return Used area width in pixels.
		 */
		const int getUsedHeight ()
		{
			return chipUsedReadout->getHeightInt ();
		}

		/**
		 * Get height of used area divided by binning factor.
		 *
		 * @return getUsedHeight() / binningVertical()
		 */
		const int getUsedHeightBinned ()
		{
			return (int) (ceil (getUsedHeight () / binningVertical ()));
		}

		/**
		 * Get X of top corner in chip coordinates.
		 *
		 * @return Chip top X corner chip coordinate.
		 */
		const int chipTopX ()
		{
			return chipSize->getXInt () + chipUsedReadout->getXInt ();
		}

		/**
		 * Get Y of top corner in chip coordinates.
		 *
		 * @return Chip top Y corner chip coordinate.
		 */
		const int chipTopY ()
		{
			return chipSize->getYInt () + chipUsedReadout->getYInt ();
		}

		virtual int setBinning (int in_vert, int in_hori);

		int center (int in_w, int in_h);

		/**
		 * Check if exposure has ended.
		 *
		 * @return 0 if there was pending exposure which ends, -1 if there wasn't any exposure, > 0 time remainnign till end of exposure,
		 * 	-3 when exposure ended because there were not any exposures in exposure que
		 */
		virtual long isExposing ();
		virtual int endExposure ();
		virtual int stopExposure ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		/**
		 * Create shutter variable,
		 */
		void createExpType ()
		{
			createValue (expType, "SHUTTER", "shutter state");
			expType->addSelVal ("LIGHT", NULL);
			expType->addSelVal ("DARK", NULL);
		}

		/**
		 * Add enumeration to shutter variable.
		 *
		 * @param enumName  Value which will be used for enumeration.
		 * @param @data     Optional data associated with enumeration.
		 */
		void addShutterType (const char *enumName, Rts2SelData *data = NULL)
		{
			expType->addSelVal (enumName, data);
		}

		/**
		 * Create value for air temperature camera sensor. Use on CCDs which
		 * can sense air temperature.
		 */
		void createTempAir ()
		{
			createValue (tempAir, "CCD_AIR", "detector air temperature");
		}

		/**
		 * Create value for CCD temperature sensor.
		 */
		void createTempCCD ()
		{
			createValue (tempCCD, "CCD_TEMP", "CCD temperature");
		}

		/**
		 * Create CCD target temperature. Used for devices which can
		 * actively regulate CCD temperature. It creates tempSet and 
		 * nightCoolTemp variables.
		 */
		void createTempSet ()
		{
			createValue (tempSet, "CCD_SET", "CCD set temperature", true, 0, CAM_WORKING, false);
			createValue (nightCoolTemp, "nightcool", "night cooling temperature", false);
			nightCoolTemp->setValueFloat (rts2_nan("f"));
			addOption ('c', NULL, 1, "night cooling temperature");
		}

		/**
		 * Returns current exposure time in seconds.
		 *
		 * @return Exposure time in seconds and fractions of seconds.
		 */
		float getExposure ()
		{
			return exposure->getValueFloat ();
		}

		/**
		 * Set exposure time.
		 */
		void setExposure (float in_exp)
		{
			exposure->setValueFloat (in_exp);
			sendValueAll (exposure);
		}

		int setSubExposure (double in_subexposure);

		double getSubExposure (void)
		{
			return subExposure->getValueDouble ();
		}

		/**
		 * Returns exposure type.
		 *
		 * @return 0 for light exposures, 1 for dark exposure.
		 */
		int getExpType ()
		{
			return expType->getValueInteger ();
		}

	public:
		virtual int deleteConnection (Rts2Conn * conn);
		/**
		 * If chip support frame transfer.
		 *
		 * @return false (default) if we don't support frame transfer, so
		 * request for readout will be handled on-site, exposure will be
		 * handed when readout ends
		 */
		virtual bool supportFrameTransfer ();

		// end of CameraChip

		Camera (int argc, char **argv);
		virtual ~ Camera (void);

		virtual int initChips ();
		virtual int initValues ();
		void checkExposures ();
		void checkReadouts ();

		virtual void deviceReady (Rts2Conn * conn);

		virtual void postEvent (Rts2Event * event);

		virtual int changeMasterState (int new_state);

		virtual int idle ();

//		virtual int changeMasterState (int new_state);
		virtual Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
		virtual int info ();

		virtual int scriptEnds ();

		virtual long camWaitExpose ();
		virtual int camStopRead ()
		{
			return endReadout ();
		}

		/**
		 * Sets camera cooling temperature.
		 *
		 * @param new_temp New cooling temperature.
		 */
		virtual int setCoolTemp (float new_temp)
		{
			return -1;
		}


		/**
		 * Called before night stars. Can be used to hook in preparing camera for night.
		 */
		virtual void beforeNight ()
		{
			if (nightCoolTemp && !isnan (nightCoolTemp->getValueFloat ()))
				setCoolTemp (nightCoolTemp->getValueFloat ());
		}
		
		/**
		 * Called when night ends. Can be used to switch off cooling. etc..
		 */
		virtual void afterNight ()
		{
		}

		/**
		 * Called when readout starts, on transition from EXPOSING to READOUT state.
		 * Need to intiate readout area, setup camera for readout etc..
		 *
		 * @return -1 on error.
		 */
		virtual int readoutStart ();

		int camReadout (Rts2Conn * conn);
		int camStopRead (Rts2Conn * conn);

		virtual int getFilterNum ();

		// focuser functions
		int setFocuser (int new_set);
		int stepFocuser (int step_count);
		int getFocPos ();

		bool isIdle ();

		/**
		 * Returns last filter number.
		 * This function is used to return last filter number, which will be saved to
		 * FITS file of the image. Problem is, that filter number can change during exposure.
		 * So the filter number, on which camera was set at the begging, is saved, and added
		 * to header of image data.
		 *
		 * @return Last filter number.
		 */
		int getLastFilterNum ()
		{
			return lastFilterNum;
		}

		/**
		 * Handles camera commands.
		 */
		virtual int commandAuthorized (Rts2Conn * conn);

		virtual int maskQueValueBopState (int new_state, int valueQueCondition);

		virtual void setFullBopState (int new_state);
};

};

#endif							 /* !__RTS2_CAMERA_CPP__ */
