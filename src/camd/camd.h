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

#define CAMERA_COOL_OFF            0
#define CAMERA_COOL_MAX            1
#define CAMERA_COOL_HOLD           2
#define CAMERA_COOL_SHUTDOWN       3

/**
 * Class which holds 2D binning informations.
 *
 * @addgroup RTS2Camera
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
 * @addgroup RTS2Camera
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevCamera:public Rts2ScriptDevice
{
	private:
		// comes from CameraChip

		void initData ();

		time_t readout_started;
		int shutter_state;

		// end of CameraChip

		Rts2Conn *exposureConn;

		char *focuserDevice;
		char *wheelDevice;

		int lastFilterNum;

		Rts2ValueFloat *exposure;

		Rts2ValueRectangle *chipSize;

		int camStartExposure (int light, float exptime);
		// when we call that function, we must be sure that either filter or wheelDevice != NULL
		int camFilter (int new_filter);

		double defaultSubExposure;
		Rts2ValueDouble *subExposure;

		Rts2ValueSelection *camFilterVal;
		Rts2ValueInteger *camFocVal;
		Rts2ValueInteger *camShutterVal;

		int getStateChip (int chip);

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
			int state_mask, int new_state, char *description);

		// chip binning
		Rts2ValueSelection *binning;

		// allowed chip data type
		Rts2ValueSelection *dataType;

		const int getDataType ()
		{
			return ((DataType *) dataType->getData ())->type;
		}

		// returns data size in bytes
		int usedPixelByteSize ()
		{
			if (getDataType () == RTS2_DATA_ULONG)
				return 4;
			return abs (getDataType () / 8);
		}

		// when chip exposure will end
		Rts2ValueTime *exposureEnd;

	protected:
		// comes from CameraChip

		int readoutLine;

		int sendLine;
		Rts2DevConnData *readoutConn;

		double pixelX;
		double pixelY;

		int nAcc;
		struct imghdr focusingHeader;

		int sendReadoutData (char *data, size_t data_size);

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
		 * Returns chip size in pixels. This method must be overloaded
		 * when camera decides to use non-standart 2D horizontal and
		 * vertical binning.
		 *
		 * @return Chip size in bytes.
		 */
		virtual int chipUsedSize ()
		{
			return (chipUsedReadout->getWidthInt () / binningHorizontal ())
				* (chipUsedReadout->getHeightInt () / binningVertical ());
		}

		/**
		 * Returns size of one line in bytes.
		 */
		int lineByteSize ()
		{
			return usedPixelByteSize () * (chipUsedReadout->getWidthInt () - chipUsedReadout->getXInt ());
		}

		char *focusingData;
		char *focusingDataTop;

		virtual int processData (char *data, size_t size);

		/**
		 * Function to do focusing. To fullfill it's task, it can
		 * use following informations:
		 *
		 * focusingData is (char *) array holding last image
		 * focusingHeader holds informations about image size etc.
		 * focusExposure is (float) with exposure setting of focussing
		 *
		 * Shall focusing need a box image, it should call box method.
		 *
		 * Return 0 if focusing should continue, !0 otherwise.
		 */
		virtual int doFocusing ();

		// end of CameraChip

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

		Rts2ValueInteger *tempRegulation;
		Rts2ValueInteger *coolingPower;
		Rts2ValueInteger *fan;

		char ccdType[64];
		char *ccdRealType;
		char serialNumber[64];

		Rts2Filter *filter;

		float nightCoolTemp;

		virtual void cancelPriorityOperations ();
		int defBinning;

		Rts2ValueDouble *rnoise;

		virtual void afterReadout ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		void createTempAir ()
		{
			createValue (tempAir, "CCD_AIR", "detector air temperature");
		}

		void createTempCCD ()
		{
			createValue (tempCCD, "CCD_TEMP", "CCD temperature");
		}

		void createTempSet ()
		{
			createValue (tempSet, "CCD_SET", "CCD set temperature");
		}

		void createTempRegulation ()
		{
			createValue (tempRegulation, "CCD_REG", "temperature regulation");
		}

		void createCoolingPower ()
		{
			createValue (coolingPower, "CCD_PWR", "cooling power");
		}

		void createCamFan ()
		{
			createValue (fan, "CCD_FAN", "fan on (1) / off (0)");
		}

	public:
		// Comes from CameraChip

		void initCameraChip ();
		void initCameraChip (int in_width, int in_height, double in_pixelX, double in_pixelY);

		void setSize (int in_width, int in_height, int in_x, int in_y)
		{
			chipSize->setInts (in_x, in_y, in_width, in_height);
			chipUsedReadout->setInts (in_x, in_y, in_width, in_height);
		}
		int getWidth ()
		{
			return chipSize->getWidthInt ();
		}
		int getHeight ()
		{
			return chipSize->getHeightInt ();
		}
		virtual int setBinning (int in_vert, int in_hori);
		virtual int box (int in_x, int in_y, int in_width, int in_height);
		int center (int in_w, int in_h);

		virtual int startExposure (int light, float exptime) = 0;

		int setExposure (float exptime, int in_shutter_state);
		virtual long isExposing ();
		virtual int endExposure ();
		virtual int stopExposure ();
		virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
		void setReadoutConn (Rts2DevConnData * dataConn);
		virtual int deleteConnection (Rts2Conn * conn);
		int getShutterState ()
		{
			return shutter_state;
		}
		virtual int endReadout ();
		void clearReadout ();
		virtual int sendFirstLine ();
		virtual int readoutOneLine ();
		/**
		 * If chip support frame transfer.
		 *
		 * @return false (default) if we don't support frame transfer, so
		 * request for readout will be handled on-site, exposure will be
		 * handed when readout ends
		 */
		virtual bool supportFrameTransfer ();

		// end of CameraChip

		Rts2DevCamera (int argc, char **argv);
		virtual ~ Rts2DevCamera (void);

		virtual int initChips ();
		virtual int initValues ();
		void checkExposures ();
		void checkReadouts ();

		virtual void deviceReady (Rts2Conn * conn);

		virtual void postEvent (Rts2Event * event);

		virtual int idle ();

		virtual int changeMasterState (int new_state);
		virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
			int other_device_type);
		virtual int ready ()
		{
			return -1;
		}
		virtual int info ();

		virtual int scriptEnds ();

		virtual int camExpose (int light, float exptime)
		{
			return startExposure (light, exptime);
		}
		virtual long camWaitExpose ();
		virtual int camStopExpose ()
		{
			return stopExposure ();
		}
		virtual int camStopRead ()
		{
			return endReadout ();
		}
		virtual int camCoolMax ()
		{
			return -1;
		}
		virtual int camCoolHold ()
		{
			return -1;
		}
		virtual int camCoolTemp (float new_temp)
		{
			return -1;
		}
		virtual int camCoolShutdown ()
		{
			return -1;
		}

		// callback functions from camera connection
		int camExpose (Rts2Conn * conn, int light, float exptime);
		int camStopExpose (Rts2Conn * conn);
		int camBox (Rts2Conn * conn, int x, int y, int width, int height);
		int camCenter (Rts2Conn * conn, int in_w, int in_h);
		int camReadout ();
		int camReadout (Rts2Conn * conn);
		// start readout & do/que exposure, that's for frame transfer CCDs
		int camReadoutExpose (Rts2Conn * conn, int light, float exptime);
		int camBinning (Rts2Conn * conn, int x_bin, int y_bin);
		int camStopRead (Rts2Conn * conn);
		int camCoolMax (Rts2Conn * conn);
		int camCoolHold (Rts2Conn * conn);
		int camCoolTemp (Rts2Conn * conn, float new_temp);
		int camCoolShutdown (Rts2Conn * conn);

		virtual int getFilterNum ();

		// focuser functions
		int setFocuser (int new_set);
		int stepFocuser (int step_count);
		int getFocPos ();

		float focusExposure;
		float defFocusExposure;

		// autofocus
		int startFocus (Rts2Conn * conn);
		int endFocusing ();

		virtual int setSubExposure (double in_subexposure);

		double getSubExposure (void)
		{
			return subExposure->getValueDouble ();
		}

		bool isIdle ();
		bool isFocusing ();

		virtual int grantPriority (Rts2Conn * conn)
		{
			if (focuserDevice)
			{
				if (conn->isName (focuserDevice))
					return 1;
			}
			return Rts2Device::grantPriority (conn);
		}

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
};
#endif							 /* !__RTS2_CAMERA_CPP__ */
