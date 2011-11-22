/* 
 * Basic camera daemon
 * Copyright (C) 2001-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CAMERA_CPP__
#define __RTS2_CAMERA_CPP__

#include <sys/time.h>
#include <time.h>

#include "scriptdevice.h"
#include "imghdr.h"

#define MAX_CHIPS  3
#define MAX_DATA_RETRY 100

/** calculateStatistics indices */
#define STATISTIC_YES     0
#define STATISTIC_ONLY    1
#define STATISTIC_NO      2

/**
 * Camera and CCD interfaces.
 */
namespace rts2camd
{

/**
 * Class which holds 2D binning informations.
 *
 * @ingroup RTS2Camera
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Binning2D: public rts2core::Rts2SelData
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
			_os << horizontal << "x" << vertical;
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
class DataType: public rts2core::Rts2SelData
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
class Camera:public rts2core::ScriptDevice
{
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

		virtual void changeMasterState (int old_state, int new_state);

		virtual int idle ();

		virtual rts2core::Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
		virtual int info ();

		virtual int killAll ();
		virtual int scriptEnds ();

		virtual long camWaitExpose ();

		/**
		 * Start/stop cooling.
		 */
		virtual int switchCooling (bool newval) { return 0; }

		/**
		 * Sets camera cooling temperature. Descendants should call
		 * Camera::setCoolTemp to make sure that the set temperature is
		 * updated.
		 *
		 * @param new_temp New cooling temperature.
		 */
		virtual int setCoolTemp (float new_temp)
		{
			tempSet->setValueDouble (new_temp);
			sendValueAll (tempSet);
			return 0;
		}

		/**
		 * Called before night stars. Can be used to hook in preparing camera for night.
		 */
		virtual void beforeNight ()
		{
			if (nightCoolTemp && !isnan (nightCoolTemp->getValueFloat ()))
			{
				switchCooling (true);
				setCoolTemp (nightCoolTemp->getValueFloat ());
			}
		}
		
		/**
		 * Called when night ends. Can be used to switch off cooling. etc..
		 */
		virtual void afterNight ()
		{
			switchCooling (false);
		}

		/**
		 * Called when readout starts, on transition from EXPOSING to READOUT state.
		 * Need to intiate readout area, setup camera for readout etc..
		 *
		 * @return -1 on error.
		 */
		virtual int readoutStart ();

		int camReadout (Rts2Conn * conn);

		// focuser functions
		int setFocuser (int new_set);
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

	protected:
		double pixelX;
		double pixelY;

		// buffer used to read data
		char* dataBuffer;
		long dataBufferSize;

		rts2core::ValueSelection *camFilterVal;
		rts2core::DoubleArray *camFilterOffsets;

		/**
		 * Return exected exposure end.
		 *
		 * @return Expected exposure end (ctime in with usec).
		 */
		double getExposureEnd ()
		{
			return exposureEnd->getValueDouble ();
		}

		/**
		 * Decrease/increase exposure end with given offset.
		 *
		 * @param off Time offset (in seconds). If positive, isExposing
		 * will run longer then nominal exposure time.
		 */
		void changeExposureEnd (double off)
		{
			exposureEnd->setValueDouble (exposureEnd->getValueDouble () + off);
		}

		/**
		 * Remove exposure connection. This should be used for cameras
		 * with long readout times before calling Camera::killAll to
		 * remove reference for connection..
		 */
		void nullExposureConn () { exposureConn = NULL; }

		rts2core::ValueDouble *pixelsSecond;

		//! number of connection waiting to be executed
		rts2core::ValueInteger *quedExpNumber;

		/**
		 * Shutter control.
		 */
		rts2core::ValueSelection *expType;

		/**
		 * Change flip of the camera. This is usually done when chaning
		 * readout channel on devices with tho channels.
		 *
		 * @param bool True if flip is changed, false if it should be
		 * reset to default.
		 */
		void changeFlip (bool change)
		{
			flip->setValueInteger (change ? !defaultFlip : defaultFlip);
			sendValueAll (flip);
		}

		/**
		 * Set flip and default flip.
		 */
		void setDefaultFlip (int _flip)
		{
			flip->setValueInteger (_flip);
			defaultFlip = _flip;
		}

		/**
		 * Returns number of exposure camera is currently taking or has taken from camera startup.
		 *
		 * @return Exposure number.
		 */
		long getExposureNumber () { return exposureNumber->getValueLong (); }

		long getScriptExposureNumber () { return scriptExposureNum->getValueLong (); }

		/**
		 * Increment exposure number. Must be called when new exposure started by "itself".
		 */
		void incExposureNumber ()
		{
			exposureNumber->inc ();
			sendValueAll (exposureNumber);
			scriptExposureNum->inc ();
			sendValueAll (scriptExposureNum);
		}

		const int getDataType ()
		{
			return ((DataType *) dataType->getData ())->type;
		}

		/**
		 * Set data type to given value. Value is index to dataType selection, created in 
		 * initDataTypes method.
		 *
		 * @param ntype new data type index
		 *
		 * @see initDataTypes()
		 */
		void setDataType (int ntype) { dataType->setValueInteger (ntype); }

		/**
		 * Allow user change data type directly. Usually data type should be changed
		 * indirectly, from some variables which triggers data type change.
		 */
		void setDataTypeWritable () { dataType->setWritable (); }

		int nAcc;
		struct imghdr *focusingHeader;

		/**
		 * Send whole image, including header.
		 *
		 * @param data Data to send.
		 * @param dataSize Size of data which will be send.
		 *
		 * @return -1 on error, 0 otherwise.
		 */
		int sendImage (char *data, size_t dataSize);

		int sendReadoutData (char *data, size_t dataSize, int chan = 0);
		
		/**
		 * Return number of bytes which are left from the image.
		 *
		 * @return Number of bytes left from the image.
		 */
		long getWriteBinaryDataSize ()
		{
			if (currentImageData == -2)
				return dataBufferSize - *((unsigned long*) shmBuffer);
			if (currentImageData < 0 && calculateStatistics->getValueInteger () == STATISTIC_ONLY)
				// end bytes
				return calculateDataSize;
			if (exposureConn)
				return exposureConn->getWriteBinaryDataSize (currentImageData);
			return 0;
		}

		/**
		 * Return number of bytes which are left from the image for the given channel.
		 *
		 * @return Number of bytes left from the image.
		 */
		long getWriteBinaryDataSize (int chan)
		{
			if (currentImageData == -2)
				return dataBufferSize - *((unsigned long*) shmBuffer);
			if (currentImageData < 0 && calculateStatistics->getValueInteger () == STATISTIC_ONLY)
				// end bytes
				return calculateDataSize;
			if (exposureConn)
				return exposureConn->getWriteBinaryDataSize (currentImageData, chan);
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
		int binningHorizontal () { return binningX->getValueInteger (); }

		/**
		 * Return vertical binning.
		 */
		int binningVertical () { return binningY->getValueInteger (); }

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
		virtual long chipUsedSize () { return getUsedWidthBinned () * getUsedHeightBinned (); }

		/**
		 * Retuns size of chip in bytes.
		 *
		 * @return Size of pixel data from current configured CCD in bytes.
		 */
		virtual long chipByteSize () { return chipUsedSize () * usedPixelByteSize (); }

		/**
		 * Returns size of one line in bytes.
		 */
		int lineByteSize () { return usedPixelByteSize () * (chipUsedReadout->getWidthInt ()); }

		virtual int processData (char *data, size_t size);

		rts2core::ValueRectangle *chipUsedReadout;

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
		// number of data channels
		rts2core::ValueInteger *dataChannels;
		// which channels are off (and which are on)
		rts2core::BoolArray *channels;

		rts2core::ValueBool *coolingOnOff;
		// temperature and others; all in deg Celsius
		rts2core::ValueFloat *tempAir;
		rts2core::ValueFloat *tempCCD;
		rts2core::ValueDoubleStat *tempCCDHistory;
		rts2core::ValueInteger *tempCCDHistoryInterval;
		rts2core::ValueInteger *tempCCDHistorySize;
		rts2core::ValueDoubleMinMax *tempSet;

		// night cooling temperature
		rts2core::ValueFloat *nightCoolTemp;

		char ccdType[64];
		char *ccdRealType;
		char serialNumber[64];

		virtual void checkQueChanges (int fakeState);

		void checkQueuedExposures ();

		void initCameraChip ();
		void initCameraChip (int in_width, int in_height, double in_pixelX, double in_pixelY);

		/**
		 * Hook to start exposure. Should send commands to camera to
		 * start exposure.  Usually maps to single call to camera API
		 * to start exposure.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int startExposure () = 0;

		virtual void afterReadout ();

		virtual int endReadout ();

		/**
		 * Read some part of the chip. If device has slow readout, and support
		 * reading by lines, this routine shall read one line. This routine <b>MUST</b>
		 * call sendReadoutData to send out data readed from chip. If it reads full buffer,
		 * it can send whole buffer (=whole image).
		 *
		 * This routine is called from camera deamon as long as it
		 * returns 0 or positive numbers. Once it returns negative
		 * number, it is not called till next exposure.
		 *
		 * @return -1 on error, -2 when readout is finished, >=0 time
		 * for next readout call.
		 */
		virtual int doReadout () = 0;

		void clearReadout ();

		void setSize (int in_width, int in_height, int in_x, int in_y)
		{
			chipSize->setInts (in_x, in_y, in_width, in_height);
			chipUsedReadout->setInts (in_x, in_y, in_width, in_height);
		}

		void setUsedHeight (int in_height)
		{
			chipUsedReadout->setHeight (in_height);
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
		virtual long suggestBufferSize () { return chipByteSize (); }

		/**
		 * Get chip width (in pixels).
		 *
		 * @return Chip width in pixels.
		 */
		const int getWidth () { return chipSize->getWidthInt (); }

		/**
		 * Get used area width (in pixels).
		 *
		 * @return Used area width in pixels.
		 */
		const int getUsedWidth () { return chipUsedReadout->getWidthInt (); }

		/**
		 * Get width of used area divided by binning factor.
		 *
		 * @return getUsedWidth() / binningHorizontal()
		 */
		const int getUsedWidthBinned () { return (int) (ceil (getUsedWidth () / binningHorizontal ())); }

		/**
		 * Returns size of single row in bytes.
		 *
		 * @return getUsedWidthBinned() * usedPixelByteSize()
		 */
		const int getUsedRowBytes () { return getUsedWidthBinned () * usedPixelByteSize (); }

		/**
		 * Get chip width (in pixels).
		 *
		 * @return Chip width in pixels.
		 */
		const int getHeight () { return chipSize->getHeightInt (); }

		/**
		 * Get X offset of used aread (in pixels)
		 *
		 */
		const int getUsedY () { return chipUsedReadout->getYInt (); }

		/**
		 * Get X offset of used aread (in pixels)
		 *
		 */
		const int getUsedX () { return chipUsedReadout->getXInt (); }

		/**
		 * Get width of used area (in pixels).
		 *
		 * @return Used area width in pixels.
		 */
		const int getUsedHeight () { return chipUsedReadout->getHeightInt (); }

		/**
		 * Get height of used area divided by binning factor.
		 *
		 * @return getUsedHeight() / binningVertical()
		 */
		const int getUsedHeightBinned () { return (int) (ceil (getUsedHeight () / binningVertical ())); }

		/**
		 * Get X of top corner in chip coordinates.
		 *
		 * @return Chip top X corner chip coordinate.
		 */
		const int chipTopX () { return chipUsedReadout->getXInt (); }

		/**
		 * Get Y of top corner in chip coordinates.
		 *
		 * @return Chip top Y corner chip coordinate.
		 */
		const int chipTopY () { return chipUsedReadout->getYInt (); }

		virtual int setBinning (int in_vert, int in_hori);

		int center (int in_w, int in_h);

		/**
		 * Check if exposure has ended.
		 *
		 * @return 0 if there was pending exposure which ends, -1 if there wasn't any exposure, > 0 time remainnign till end of exposure,
		 * 	-3 when exposure ended because there were not any exposures in exposure que
		 */
		virtual long isExposing ();

		/**
		 * Called after isExposing returned 0.
		 * 
		 * @return -1 on error, 0 on success.
		 */
		virtual int endExposure ();

		/**
		 * Hook to stop exposure.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int stopExposure ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void valueChanged (rts2core::Value *changed_value);

		/**
		 * Create shutter variable,
		 */
		void createExpType ()
		{
			createValue (expType, "SHUTTER", "shutter state", true, RTS2_VALUE_WRITABLE);
			expType->addSelVal ("LIGHT", NULL);
			expType->addSelVal ("DARK", NULL);
		}

		/**
		 * Add enumeration to shutter variable.
		 *
		 * @param enumName  Value which will be used for enumeration.
		 * @param @data     Optional data associated with enumeration.
		 */
		void addShutterType (const char *enumName, rts2core::Rts2SelData *data = NULL) { expType->addSelVal (enumName, data); }

		/**
		 * CCDs with multiple data channels.
		 */
		void createDataChannels ()
		{ 
			createValue (dataChannels, "DATA_CHANNELS", "total number of data channels", true);
			createValue (channels, "CHAN", "channels on/off", true, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE | RTS2_DT_SIMPLE_ARRAY, CAM_WORKING);
		}

		void setNumChannels (int num)
		{
			channels->clear ();
			for (int i = 0; i < num; i++)
				channels->addValue (true);
		}

		/**
		 * Create value for air temperature camera sensor. Use on CCDs which
		 * can sense air temperature.
		 */
		void createTempAir () { createValue (tempAir, "CCD_AIR", "detector air temperature"); }

		/**
		 * Create value for CCD temperature sensor.
		 */
		void createTempCCD () { createValue (tempCCD, "CCD_TEMP", "CCD temperature"); }

		/**
		 * Create statistics value to hold array of temperatures histories.
		 */
		void createTempCCDHistory () {
			createValue (tempCCDHistory, "ccd_temp_history", "history of ccd temperatures", false);
			createValue (tempCCDHistoryInterval, "ccd_temp_interval", "interval in seconds between repeative queries of CCD temperature", false);
			createValue (tempCCDHistorySize, "ccd_temp_size", "interval in seconds between repeative queries of CCD temperature", false);

			tempCCDHistoryInterval->setValueInteger (2);
			tempCCDHistorySize->setValueInteger (20);
		}

		/**
		 * Add temperature to temprature history.
		 */
		void addTempCCDHistory (float temp);

		/**
		 * Method called every tempCCDHistoryInterval seconds to check CCD temperature.
		 */
		virtual void temperatureCheck () {};

		/**
		 * Create CCD target temperature. Used for devices which can
		 * actively regulate CCD temperature. It creates tempSet and 
		 * nightCoolTemp variables.
		 */
		void createTempSet ()
		{
			createValue (coolingOnOff, "COOLING", "camera cooling start/stop", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF, CAM_WORKING);
			createValue (tempSet, "CCD_SET", "CCD set temperature", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
			createValue (nightCoolTemp, "nightcool", "night cooling temperature", false, RTS2_VALUE_WRITABLE);
			nightCoolTemp->setValueFloat (rts2_nan("f"));
			addOption ('c', NULL, 1, "night cooling temperature");
		}

		/**
		 * Returns current exposure time in seconds.
		 *
		 * @return Exposure time in seconds and fractions of seconds.
		 */
		double getExposure () { return exposure->getValueDouble (); }

		/**
		 * Set exposure minimal and maximal times.
		 */
		void setExposureMinMax (double exp_min, double exp_max);

		/**
		 * Set exposure time.
		 */
		virtual void setExposure (double exp) { exposure->setValueDouble (exp); }

		/**
		 * Returns exposure type.
		 *
		 * @return 0 for light exposures, 1 for dark exposure.
		 */
		int getExpType () { return expType->getValueInteger ();	}

		/**
		 * Set camera filter.
		 *
		 * @param @new_filter   new filter number
		 */
		virtual int setFilterNum (int new_filter);
		virtual int getFilterNum ();

		void offsetForFilter (int new_filter);

		int getCamFilterNum () { return camFilterVal->getValueInteger (); }

	private:
		long readoutPixels;
		double timeReadoutStart;

		// connection which requries data to be send after end of exposure
		Rts2Conn *exposureConn;

		rts2core::ValueDoubleMinMax *exposure;

		// shared memory identifier
		int sharedMemId;
		// shared memory buffer - this include size (unsigned long) and image header structure
		char* shmBuffer;

		// number of exposures camera takes
		rts2core::ValueLong *exposureNumber;
		// exposure number inside script
		rts2core::ValueLong *scriptExposureNum;
		rts2core::ValueBool *waitingForEmptyQue;
		rts2core::ValueBool *waitingForNotBop;

		rts2core::ValueInteger *binningX;
		rts2core::ValueInteger *binningY;

		char *focuserDevice;
		char *wheelDevice;

		int lastFilterNum;

		int currentImageData;

		// whenewer statistics should be calculated
		rts2core::ValueSelection *calculateStatistics;

		// image parameters
		rts2core::ValueDouble *average;
		rts2core::ValueDouble *min;
		rts2core::ValueDouble *max;
		rts2core::ValueDouble *sum;

		rts2core::ValueLong *computedPix;

		// update statistics
		template <typename t> int updateStatistics (t *data, size_t dataSize)
		{
			long double tSum = 0;
			double tMin = min->getValueDouble ();
			double tMax = max->getValueDouble ();
			int pixNum = 0;
			t *tData = data;
			while (((char *) tData) < ((char *) data) + dataSize)
			{
				t tD = *tData;
				tSum += tD;
				if (tD < tMin)
					tMin = tD;
				if (tD > tMax)
				  	tMax = tD;
				tData++;
				pixNum++;
			}
			sum->setValueDouble (sum->getValueDouble () + tSum);
			if (tMin < min->getValueDouble ())
				min->setValueDouble (tMin);
			if (tMax > max->getValueDouble ())
				max->setValueDouble (tMax);
			return pixNum;
		}

								 // DARK of LIGHT frames
		rts2core::ValueInteger *flip;
		bool defaultFlip;

		rts2core::ValueDouble *xplate;
		rts2core::ValueDouble *yplate;
		double defaultXplate;
		double defaultYplate;

		int setPlate (const char *arg);
		void setDefaultPlate (double x, double y);

		rts2core::ValueRectangle *chipSize;

		int camStartExposure ();
		int camStartExposureWithoutCheck ();

		rts2core::ValueInteger *camFocVal;
		rts2core::ValueDouble *rotang;

		int getStateChip (int chip);

		// chip binning
		rts2core::ValueSelection *binning;

		// allowed chip data type
		rts2core::ValueSelection *dataType;

		// when chip exposure will end
		rts2core::ValueTime *exposureEnd;

		// filter wheel is moving
		rts2core::ValueBool *filterMoving;
		rts2core::ValueBool *focuserMoving;

		// set chipUsedSize size
		int box (int _x, int _y, int _width, int _height, rts2core::ValueRectangle *retv = NULL);

		// callback functions from camera connection
		int camExpose (Rts2Conn * conn, int chipState, bool fromQue);
		int camBox (Rts2Conn * conn, int x, int y, int width, int height);
		int camCenter (Rts2Conn * conn, int in_w, int in_h);

		/**
		 * Return physical channel associated with
		 * data channel number.
		 *
		 * @param ch channel number.
		 */
		int getPhysicalChannel (int ch);

		void startImageData (Rts2Conn * conn);

		/**
		 * @param chan    channel number (0..nchan)
		 * @param pchan   physical channel number (any number)
		 */
		int sendFirstLine (int chan, int pchan);

		// if true, send command OK after exposure is started
		bool sendOkInExposure;

		long calculateDataSize;

		void setFilterOffsets (char *opt);
};

}

#endif							 /* !__RTS2_CAMERA_CPP__ */
